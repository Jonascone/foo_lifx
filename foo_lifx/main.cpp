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

bool playing = true;

float offset = 0.2f;
float smoother = 0.f;
double next_tick = 0;

const int fft_size = 1024;

// Shamefully nicked this and a lot of other code from here:
// http://stackoverflow.com/questions/20408388/how-to-filter-fft-data-for-audio-visualisation#answer-20584591

const size_t aweight_length = 34;

const float aweight_frequency[] = {
	10, 12.5, 16, 20,
	25, 31.5, 40, 50,
	63, 80, 100, 125,
	160, 200, 250, 315,
	400, 500, 630, 800,
	1000, 1250, 1600, 2000,
	2500, 3150, 4000, 5000,
	6300, 8000, 10000, 12500,
	16000, 20000
};

const float aweight_decibels[] = {
	-70.4, -63.4, -56.7, -50.5,
	-44.7, -39.4, -34.6, -30.2,
	-26.2, -22.5, -19.1, -16.1,
	-13.4, -10.9, -8.6, -6.6,
	-4.8, -3.2, -1.9, -0.8,
	0.0, 0.6, 1.0, 1.2,
	1.3, 1.2, 1.0, 0.5,
	-0.1, -1.1, -2.5, -4.3,
	-6.6, -9.3
};

float get_frequency_weight(float xx) {
	const float *x = aweight_frequency;
	const float *y = aweight_decibels;

	float result = 0.0;
	boolean found = false;

	if (x[0] > xx) {
		result = y[0];
		found = true;
	}

	if (!found) {
		for (int i = 1; i < aweight_length; i++) {
			if (x[i] > xx) {
				result = y[i - 1] + ((xx - x[i - 1]) / (x[i] - x[i - 1])) * (y[i] - y[i - 1]);
				found = true;
				break;
			}
		}
	}

	if (!found) {
		result = y[aweight_length - 1];
	}

	return result;
}

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
			next_tick = 0;
		}
		
		void on_playback_pause(bool state) {
			playing = !playing;
			lifx_send<lifx::message::light::SetColor>(default_colour);
		}

		void on_playback_stop(play_control::t_stop_reason reason) { 
			if (reason == play_control::t_stop_reason::stop_reason_starting_another) return;
			on_playback_pause(true);
		}

		void on_playback_seek(double seek) {
			next_tick = 0;
		}
	} *play_callback = nullptr;

	class lifx_callback : public playback_stream_capture_callback {
	protected:
		// Colour cycle
		float cycle = 0.f;

		float dB(float x) {
			if (!x) return 0.f;
			return 10 * log10f(x);
		}

		float get_spectrum(double time = 0.0) {
			if (!time) {
				visualiser->get_absolute_time(time);
			}

			audio_chunk_impl_t<> chunk;
			if (!visualiser->get_spectrum_absolute(chunk, time, fft_size)) return 0.f;

			const size_t chunk_size = chunk.get_data_size();
			if (!chunk_size) return 0.f;

			const audio_sample *chunk_data = chunk.get_data();

			float step = chunk.get_sample_rate() / fft_size;
			float max = 0.f;
			float min = 0.f;
			bool min_found = false;

			float aweight_values[fft_size] = { 0 };
			
			for (size_t i = chunk_size; i--;) {
				float frequency = i * step;
				if (frequency > 18000.f || chunk_data[i] < 0.05f) continue;

				float value = dB(chunk_data[i]) + get_frequency_weight(frequency / 2);
				if (value > max) {
					max = value;
				}

				if (!min_found || value < min) {
					min = value;
				}

				aweight_values[i] = value;
			}

			float range = max - min;
			float scale = range + 0.00001;
			float average = 0.f;
			size_t average_ct = 0;
			for (size_t i = chunk_size; i--;) {
				if (i * step > 18000.f || chunk_data[i] < 0.05f) continue;
				average += ((aweight_values[i] - min) / scale);
				++average_ct;
			}

			if (!average_ct) return 0.f;
			return max(0.f, average / average_ct);
		}
				
		float offset_spectrum(double start_offset = 0.0) {
			double cur_time;
			visualiser->get_absolute_time(cur_time);
			
			cur_time += start_offset;

			double tgt_time = cur_time + offset;

			double sample_step = 0.01 * offset;
			for (; cur_time <= tgt_time; cur_time += sample_step) {
				float value = get_spectrum(cur_time);
				smoother = (smoother * sample_step) + (value * (1.0 - sample_step));
			}

			return smoother;
		}

	public:
		void on_chunk(const audio_chunk&) {
			if (!playing) {
				// This callback can occur after the play callback so we need to reset the light colour here too
				lifx_send<lifx::message::light::SetColor>(default_colour);
				return;
			}
			
			double cur_tick;
			visualiser->get_absolute_time(cur_tick);
									
			if (next_tick < cur_tick) {
				offset = cfg_lifx_offset / 1000.f;
				next_tick = cur_tick + offset;
				
				float amplitude = offset_spectrum(offset * 1.1f);

				float intensity = 0.f;
				switch (cfg_lifx_intensity) {
				case 1:
					intensity = 65535 * (0.25f + amplitude * 0.75f);
					break;
				case 2:
					intensity = 65535 * (0.125f + amplitude * 0.875f);
					break;
				case 3:
					intensity = 65535 * amplitude;
					break;
				default:
					intensity = 65535 * (0.5f + amplitude * 0.5f); 
				}

				float brightness_percentage = static_cast<float>(cfg_lifx_brightness) / 100.f;
				uint16_t brightness = min(65535, static_cast<uint16_t>(intensity * brightness_percentage));
				
				lifx::message::light::SetColor msg;
				if (cfg_lifx_cycle_enabled) {
					cycle = (cycle < 180 ? cycle + 180 / static_cast<float>(cfg_lifx_cycle_speed / offset): 0);
					msg = {
						static_cast<uint16_t>(65535 * sin(cycle * std::_Pi / 180)),
						65535,
						brightness,
						3500,
						cfg_lifx_offset // Duration
					};
				}
				else {
					msg = {
						static_cast<uint16_t>(65535 * (static_cast<float>(cfg_lifx_hue) / 360)),
						static_cast<uint16_t>(65535 * (static_cast<float>(cfg_lifx_saturation) / 100)),
						brightness,
						3500,
						cfg_lifx_offset // Duration
					};
				}

				lifx_send<lifx::message::light::SetColor>(msg);
								
				if (debug) {
					char buff[128] = { '\0' };
					sprintf(buff, "B: %d B: %f, A: %f, O: %f", brightness, static_cast<float>(brightness) / 65535.f, amplitude, offset);
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