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

std::vector<std::array<uint8_t, 8>> g_lightbulbs;
lifx::LifxClient g_client;

class lifx_initquit : public initquit {
protected:
	template <typename T>
	static void run_command(const T& message) {
		for (auto mac_address : g_lightbulbs) {
			g_client.Send<T>(message, mac_address.data());
		}
		while (g_client.WaitingToSend()) g_client.RunOnce();
	}

	class audio_callback : public playback_stream_capture_callback {
	protected:
		// Peak smoothing
		float smoothing = 0.99;
		float smoother = 0.0;

		// Colour cycle
		clock_t cycle_delay = 0;
		float cycle = 0.0;
		bool debug = false;
	public:
		void on_chunk(const audio_chunk &chunk) {
			smoother = (chunk.get_peak() * smoothing) + (smoother * (1.0f - smoothing));

			clock_t cur_tick = clock();
						
			if (cycle_delay < cur_tick) {
				float brightness_percentage = static_cast<float>(cfg_lifx_brightness) / 100.f;
				uint16_t brightness = min(65535, (29127 * (1 + smoother)) * brightness_percentage);

				cycle_delay = cur_tick + 100;

				lifx::message::light::SetColor msg;

				if (cfg_lifx_cycle_enabled) {
					cycle = (cycle < 180 ? cycle + (180 / static_cast<float>(cfg_lifx_cycle_speed) * 0.1) : 0);
					msg = { 
						static_cast<uint16_t>(65535 * sin(cycle * std::_Pi / 180)), 
						65535, 
						brightness, 
						3500 
					};
				}
				else {
					msg = { 
						static_cast<uint16_t>(65535 * (static_cast<float>(cfg_lifx_hue) / 360)), 
						static_cast<uint16_t>(65535 * (static_cast<float>(cfg_lifx_saturation) / 100)), 
						brightness, 
						3500 
					};
				}
				
				msg.duration = 90;
				run_command<lifx::message::light::SetColor>(msg);

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

		g_client.RegisterCallback<lifx::message::device::StateService>(
			[](const lifx::Header& header, const lifx::message::device::StateService& msg)
		{
			if (msg.service == lifx::SERVICE_UDP)
			{
				std::array<uint8_t, 8> mac_address;

				// Copy contents of mac address
				for (size_t i = 0; i < 8; ++i) {
					mac_address[i] = header.target[i];
				}

				g_lightbulbs.push_back(mac_address);

				g_client.Send<lifx::message::light::Get>(mac_address.data());
			}
		});

		g_client.Broadcast<lifx::message::device::GetService>({});

		// Stop searching for bulbs after 5 seconds
		time_t timeout = time(nullptr) + 5;

		while (true && time(nullptr) < timeout) {
			g_client.RunOnce();
			if (g_lightbulbs.size()) break;
		}

		run_command<lifx::message::light::SetColor>(lifx::message::light::SetColor { 58275, 0, 65535, 5500 });

		lifx::message::device::SetPower msg { 65535 };
		run_command<lifx::message::device::SetPower>(msg);
		// unknown or v1 products sometimes have issues setting power, so
		// we have to send it twice
		run_command<lifx::message::device::SetPower>(msg);

		static_api_ptr_t<playback_stream_capture>()->add_callback(callback = new audio_callback());
	}

	void on_quit() {
		if (cfg_lifx_enabled) {
			run_command<lifx::message::device::SetPower>({});
			run_command<lifx::message::light::SetColor>(lifx::message::light::SetColor { 58275, 0, 65535, 5500 });
		}

		if (callback) {
			static_api_ptr_t<playback_stream_capture>()->remove_callback(callback);
			delete callback;
		}
	}
};

static initquit_factory_t<lifx_initquit> lifx_initquit_factory;