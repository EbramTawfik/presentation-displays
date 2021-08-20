#include "run_loop.h"

#include <windows.h>

#include <algorithm>
//
//#define min(a, b) ((a) < (b) ? (a) : (b))
//#define max(a, b) ((a) > (b) ? (a) : (b))

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

RunLoop::RunLoop() {}

RunLoop::~RunLoop() {}

void RunLoop::Run()
{
  bool keep_running = true;
  TimePoint next_flutter_event_time = TimePoint::clock::now();
  while (keep_running)
  {
    std::chrono::nanoseconds wait_duration =
        max(std::chrono::nanoseconds(0),
            next_flutter_event_time - TimePoint::clock::now());
    ::MsgWaitForMultipleObjects(
        0, nullptr, FALSE, static_cast<DWORD>(wait_duration.count() / 1000),
        QS_ALLINPUT);
    bool processed_events = false;
    MSG message;
    // All pending Windows messages must be processed; MsgWaitForMultipleObjects
    // won't return again for items left in the queue after PeekMessage.
    while (::PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
    {
      processed_events = true;
      if (message.message == WM_QUIT)
      {
        keep_running = false;
        break;
      }
      ::TranslateMessage(&message);
      ::DispatchMessage(&message);
      // Allow Flutter to process messages each time a Windows message is
      // processed, to prevent starvation.
      next_flutter_event_time =
          min(next_flutter_event_time, ProcessFlutterMessages());
    }
    // If the PeekMessage loop didn't run, process Flutter messages.
    if (!processed_events)
    {
      next_flutter_event_time =
          min(next_flutter_event_time, ProcessFlutterMessages());
    }
  }
}

void RunLoop::RegisterFlutterInstance(
    flutter::FlutterEngine *flutter_instance)
{
  flutter_instances_.insert(flutter_instance);
}

void RunLoop::UnregisterFlutterInstance(
    flutter::FlutterEngine *flutter_instance)
{
  flutter_instances_.erase(flutter_instance);
}

RunLoop::TimePoint RunLoop::ProcessFlutterMessages()
{
  //TimePoint next_event_time = TimePoint::max();
  TimePoint next_event_time = TimePoint();
  for (auto instance : flutter_instances_)
  {
    std::chrono::nanoseconds wait_duration = instance->ProcessMessages();
    //if (wait_duration != std::chrono::nanoseconds::max())
    if (wait_duration != std::chrono::nanoseconds())
    {
      next_event_time =
          min(next_event_time, TimePoint::clock::now() + wait_duration);
    }
  }
  return next_event_time;
}
