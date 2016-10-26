// foo_lifx.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

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
		float smoothing = 0.6;
		float smoother = 0;

		// Colour cycle
		time_t cycle_delay = 0;
		float cycle_duration = 30;
		float cycle = 0.0;

		uint16_t last_brightness = 65535;
	public:
		void on_chunk(const audio_chunk &chunk) {
			float peak = min(1.f, chunk.get_peak() * 1);
			smoother = smoothing * smoother + ((1 - smoothing) * peak);

			time_t cur_tick = clock();

			if (smoother > 0.15) {
				last_brightness = min(65535, 13107 + static_cast<uint16_t>(65535 * peak));
			}

			if (cycle_delay < cur_tick) {
				cycle_delay = cur_tick + 105;
				cycle = (cycle < 180 ? cycle + (180 / cycle_duration * 0.1) : 0);
				lifx::message::light::SetColor msg{ 65535 * sin(cycle * std::_Pi / 180), 65535, last_brightness, 3500 };
				msg.duration = 100;
				run_command<lifx::message::light::SetColor>(msg);
			}
		}
	} *callback = nullptr;
public:
	void on_init() {
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

		while (true) {
			g_client.RunOnce();
			if (g_lightbulbs.size()) break;
		}

		run_command<lifx::message::light::SetColor>(lifx::message::light::SetColor{ 58275, 0, 65535, 5500 });

		lifx::message::device::SetPower msg{ 65535 };
		run_command<lifx::message::device::SetPower>(msg);
		// unknown or v1 products sometimes have issues setting power, so
		// we have to send it twice
		run_command<lifx::message::device::SetPower>(msg);

		static_api_ptr_t<playback_stream_capture>()->add_callback(callback = new audio_callback());
	}

	void on_quit() {
		run_command<lifx::message::device::SetPower>({});
		run_command<lifx::message::light::SetColor>(lifx::message::light::SetColor{ 58275, 0, 65535, 5500 });

		static_api_ptr_t<playback_stream_capture>()->remove_callback(callback);
		delete callback;
	}
};

static initquit_factory_t<lifx_initquit> lifx_initquit_factory;