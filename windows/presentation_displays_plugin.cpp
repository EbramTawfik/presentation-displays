#include "include/presentation_displays/presentation_displays_plugin.h"

// This must be included before many other Windows headers.
#include <windows.h>

// For getPlatformVersion; remove unless needed for your plugin implementation.
#include <VersionHelpers.h>
#include <flutter/dart_project.h>
#include <flutter/flutter_view_controller.h>
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <map>
#include <memory>
#include <sstream>

#include "flutter_window.h"
#include "run_loop.h"

namespace
{

	class PresentationDisplaysPlugin : public flutter::Plugin
	{
	public:
		static void RegisterWithRegistrar(flutter::PluginRegistrarWindows* registrar);

		PresentationDisplaysPlugin();

		virtual ~PresentationDisplaysPlugin();

	private:
		// Called when a method is called on this plugin's channel from Dart.
		void HandleMethodCall(
			const flutter::MethodCall<flutter::EncodableValue>& method_call,
			std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
	};

	std::vector<RECT> screens;

	BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor,
		HDC hdcMonitor,
		LPRECT lprcMonitor,
		LPARAM dwData)
	{
		MONITORINFO info;
		info.cbSize = sizeof(info);
		if (GetMonitorInfo(hMonitor, &info))
		{
			RECT r = {};
			r.bottom = info.rcMonitor.bottom;
			r.left = info.rcMonitor.left;
			r.right = info.rcMonitor.right;
			r.top = info.rcMonitor.top;
			screens.push_back(r);
			std::cout << "Monitor x: " << std::abs(info.rcMonitor.left - info.rcMonitor.right)
				<< " y: " << std::abs(info.rcMonitor.top - info.rcMonitor.bottom)
				<< std::endl;
		}
		return TRUE; // continue enumerating
	}

	void showScreen(int index)
	{
		RunLoop run_loop;

		flutter::DartProject project(L"data");

		FlutterWindow window(&run_loop, project);
		Win32Window::Point origin(10, 10);
		Win32Window::Size size(1280, 720);
		if (!window.CreateAndShow(L"example 2", origin, size))
		{
			return;
		}
		window.SetQuitOnClose(true);

		run_loop.Run();
	}
	std::string toString(WCHAR* v)
	{
		std::wstring ws(v);
		std::string st(ws.begin(), ws.end());
		return st;
	}

	// static
	void PresentationDisplaysPlugin::RegisterWithRegistrar(
		flutter::PluginRegistrarWindows* registrar)
	{
		auto channel =
			std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
				registrar->messenger(), "presentation_displays_plugin",
				&flutter::StandardMethodCodec::GetInstance());

		auto plugin = std::make_unique<PresentationDisplaysPlugin>();

		channel->SetMethodCallHandler(
			[plugin_pointer = plugin.get()](const auto& call, auto result)
		{
			plugin_pointer->HandleMethodCall(call, std::move(result));
		});

		registrar->AddPlugin(std::move(plugin));
	}

	PresentationDisplaysPlugin::PresentationDisplaysPlugin() {}

	PresentationDisplaysPlugin::~PresentationDisplaysPlugin() {}

	void PresentationDisplaysPlugin::HandleMethodCall(
		const flutter::MethodCall<flutter::EncodableValue>& method_call,
		std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result)
	{
		if (method_call.method_name().compare("getPlatformVersion") == 0)
		{
			std::ostringstream version_stream;
			version_stream << "Windows ";
			if (IsWindows10OrGreater())
			{
				version_stream << "10+";
			}
			else if (IsWindows8OrGreater())
			{
				version_stream << "8";
			}
			else if (IsWindows7OrGreater())
			{
				version_stream << "7";
			}
			result->Success(flutter::EncodableValue(version_stream.str()));
		}
		else if (method_call.method_name().compare("listDisplay") == 0)
		{
			screens.clear();
			EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);
			for (size_t i = 0; i < screens.size(); i++)
			{
				std::cout << i << " " << screens[i].left << "\n";
			}
			showScreen(3);
			std::ostringstream version_stream;
			DISPLAY_DEVICE dd;
			dd.cb = sizeof(dd);
			int deviceIndex = 0;
			int id = 0;
			version_stream << "[";
			while (EnumDisplayDevices(0, deviceIndex, &dd, 0))
			{
				std::wstring deviceName = dd.DeviceName;
				int monitorIndex = 0;
				DISPLAY_DEVICE ddMonitor;
				ddMonitor.cb = sizeof(ddMonitor);

				while (EnumDisplayDevices(deviceName.c_str(), monitorIndex, &ddMonitor, 0))
				{
					std::string name = toString(ddMonitor.DeviceName);

					std::cout << "Device = " << toString(ddMonitor.DeviceID) << " " << name << " " << toString(ddMonitor.DeviceString) << " " << std::endl;

					version_stream << "{\"displayId\" : " << id++ << ", \"name\": \"" << name << "\"},";
					++monitorIndex;
				}
				++deviceIndex;
			}

			std::string res = version_stream.str();
			res.pop_back();
			res += "]";
			std::cout << res;
			//"[{\"name\":\"Display 1\" ,\"displayId\":1}]"
			result->Success(flutter::EncodableValue(res));
		}
		else
		{
			result->NotImplemented();
		}
	}

} // namespace

void PresentationDisplaysPluginRegisterWithRegistrar(
	FlutterDesktopPluginRegistrarRef registrar)
{
	PresentationDisplaysPlugin::RegisterWithRegistrar(
		flutter::PluginRegistrarManager::GetInstance()
		->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
