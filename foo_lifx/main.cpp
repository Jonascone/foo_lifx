// foo_lifx.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "preferences.h"

// Declaration of your component's version information
// Since foobar2000 v1.0 having at least one of these in your DLL is mandatory to let the troubleshooter tell different versions of your component apart.
// Note that it is possible to declare multiple components within one DLL, but it's strongly recommended to keep only one declaration per DLL.
// As for 1.1, the version numbers are used by the component update finder to find updates; for that to work, you must have ONLY ONE declaration per DLL. If there are multiple declarations, the component is assumed to be outdated and a version number of "0" is assumed, to overwrite the component with whatever is currently on the site assuming that it comes with proper version numbers.
DECLARE_COMPONENT_VERSION("Lifx for Foobar 2000", "0.1", "");


// This will prevent users from renaming your component around (important for proper troubleshooter behaviors) or loading multiple instances of it.
VALIDATE_COMPONENT_FILENAME("foo_lifx.dll");

bool debug = false;

lifx::LifxClient lifx_client;
std::vector<std::array<uint8_t, 8>> lifx_lightbulbs;
service_ptr_t<visualisation_stream> visualiser;
const lifx::message::light::SetColor default_colour { 58275, 0, 65535, 5500, 250 };

bool playing = false;
float smoother = 0.f;

template <typename T>
void lifx_send(const T& message) {
	for (auto mac_address : lifx_lightbulbs) {
		lifx_client.Send<T>(message, mac_address.data());
	}
	while (lifx_client.WaitingToSend()) lifx_client.RunOnce();
}

class lifx_initquit : public initquit {
protected:
	class lifx_play_callback : public play_callback_impl_base {
	public:
		void on_playback_new_track(metadb_handle_ptr) {
			playing = true;
			smoother = 0.f;
		}
		
		void on_playback_pause(bool state) {
			playing = !playing;
			lifx_send<lifx::message::light::SetColor>(default_colour);
		}

		void on_playback_stop(play_control::t_stop_reason reason) { 
			if (reason == play_control::t_stop_reason::stop_reason_starting_another) return;
			on_playback_pause(true);
		}
	} *play_callback = nullptr;

	class lifx_callback : public playback_stream_capture_callback {
	protected:
		// Peak smoothing
		float smoothing = 0.99f;

		// Next time we can send a colour change
		clock_t next_tick = 0;

		// Colour cycle
		float cycle = 0.f;

		float get_spectrum(unsigned int fft_size = 1024) {
			double time;
			visualiser->get_absolute_time(time);

			audio_chunk_impl_t<> chunk;
			visualiser->get_spectrum_absolute(chunk, time, fft_size);

			size_t chunk_size = chunk.get_data_size();
			if (!chunk_size) return 0.f;

			float step = (static_cast<float>(chunk.get_sample_rate()) / static_cast<float>(fft_size));

			// Work out the highest index for low and mid frequency ranges
			size_t lo_idx = static_cast<size_t>(250.f / step);
			size_t mi_idx = static_cast<size_t>(2000.f / step);

			// Sort the spectrum data into low, mid and high frequency ranges
			float lo_fq = 0.f;
			float mi_fq = 0.f;
			float hi_fq = 0.f;

			size_t lo_ct = 0;
			size_t mi_ct = 0;
			size_t hi_ct = 0;

			const audio_sample *chunk_data = chunk.get_data();
			for (size_t i = chunk_size; i--;) {
				const audio_sample data = chunk_data[i];
				if (data < 0.1f) continue; // Filter out any noise

				if (i > mi_idx) {
					++hi_ct;
					hi_fq += data;
				}
				else if (i > lo_idx) {
					++mi_ct;
					mi_fq += data;
				}
				else {
					++lo_ct;
					lo_fq += data;
				}
			}

			// Work out the average of each frequency range
			if (lo_fq && lo_ct)
				lo_fq /= lo_ct;

			if (mi_fq && mi_ct)
				mi_fq /= mi_ct;

			if (hi_fq && hi_ct)
				hi_fq /= hi_ct;

			return lo_fq + mi_fq + hi_fq;
		}

	public:
		void on_chunk(const audio_chunk&) {
			if (!playing) {
				// This callback can occur after the play callback so we need to reset the light colour here too
				lifx_send<lifx::message::light::SetColor>(default_colour);
				return;
			}

			smoother = (get_spectrum() * smoothing) + (smoother * (1.f - smoothing));

			clock_t cur_tick = clock();
			if (next_tick < cur_tick) {
				next_tick = cur_tick + 100;

				float intensity = 0.f;
				switch (cfg_lifx_intensity) {
				case 1:
					intensity = (32768 * (0.125f + smoother * 1.875f));
					break;
				case 2:
					intensity = (32768 * (0.25f + smoother * 1.75f));
					break;
				default:
					intensity = (32768 * (0.5f + smoother * 1.5f));

				}

				float brightness_percentage = static_cast<float>(cfg_lifx_brightness) / 100.f;
				uint16_t brightness = min(65535, static_cast<uint16_t>(intensity * brightness_percentage));
				
				lifx::message::light::SetColor msg;
				if (cfg_lifx_cycle_enabled) {
					cycle = (cycle < 180 ? cycle + (180 / static_cast<float>(cfg_lifx_cycle_speed) * 0.1f) : 0);
					msg = {
						static_cast<uint16_t>(65535 * sin(cycle * std::_Pi / 180)),
						65535,
						brightness,
						3500,
						90 // Duration
					};
				}
				else {
					msg = {
						static_cast<uint16_t>(65535 * (static_cast<float>(cfg_lifx_hue) / 360)),
						static_cast<uint16_t>(65535 * (static_cast<float>(cfg_lifx_saturation) / 100)),
						brightness,
						3500,
						90 // Duration
					};
				}

				lifx_send<lifx::message::light::SetColor>(msg);

				if (debug) {
					char buff[128] = { '\0' };
					sprintf(buff, "B: %f, S: %f", static_cast<float>(brightness) / 65535.f, smoother);
					console::print(buff);
				}
			}
		}
	} *callback = nullptr;
public:
	void on_init() {
		if (!cfg_lifx_enabled) return;

		lifx_client.RegisterCallback<lifx::message::device::StateService>(
			[](const lifx::Header& header, const lifx::message::device::StateService& msg)
		{
			if (msg.service == lifx::SERVICE_UDP)
			{
				std::array<uint8_t, 8> mac_address;

				// Copy contents of mac address
				for (size_t i = 0; i < 8; ++i) {
					mac_address[i] = header.target[i];
				}

				lifx_lightbulbs.push_back(mac_address);

				lifx_client.Send<lifx::message::light::Get>(mac_address.data());
			}
		});

		lifx_client.Broadcast<lifx::message::device::GetService>({});

		// Stop searching for bulbs after 5 seconds if no bulb is found
		time_t timeout = time(nullptr) + 5;
		while (time(nullptr) < timeout) {
			lifx_client.RunOnce();
			if (lifx_lightbulbs.size()) break;
		}

		lifx_send<lifx::message::light::SetColor>(default_colour);
		console::printf("%d", default_colour.duration);
		lifx::message::device::SetPower msg { 65535 };
		lifx_send<lifx::message::device::SetPower>(msg);
		// unknown or v1 products sometimes have issues setting power, so
		// we have to send it twice
		lifx_send<lifx::message::device::SetPower>(msg);

		// playlist_callback_impl_base register/unregisters itself automatically
		play_callback = new lifx_play_callback();
		static_api_ptr_t<visualisation_manager>()->create_stream(visualiser, visualisation_manager::KStreamFlagNewFFT);
		static_api_ptr_t<playback_stream_capture>()->add_callback(new lifx_callback());
	}

	void on_quit() {
		if (cfg_lifx_enabled) {
			lifx_send<lifx::message::device::SetPower>({});
			lifx_send<lifx::message::light::SetColor>(default_colour);
		}

		if (play_callback) delete play_callback;
		if (callback) delete callback;
	}
};

static initquit_factory_t<lifx_initquit> lifx_initquit_factory;