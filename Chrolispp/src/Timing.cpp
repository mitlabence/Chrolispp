#include "Timing.hpp"

#if defined(_WIN32)
#pragma comment(lib, "Winmm.lib")
#include <windows.h>

void Timing::precise_sleep_for(std::chrono::milliseconds ms) {
  // Request 1 ms timer resolution on Windows
  timeBeginPeriod(1);
  Sleep(static_cast<DWORD>(ms.count()));
  timeEndPeriod(1);
}

#elif defined(__linux__)
#include <time.h>

void Timing::precise_sleep_for(std::chrono::milliseconds ms) {
  timespec ts;
  ts.tv_sec = ms.count() / 1000;
  ts.tv_nsec = (ms.count() % 1000) * 1'000'000;
  clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, nullptr);
}

#elif defined(__APPLE__)
#include <time.h>

void Timing::precise_sleep_for(std::chrono::milliseconds ms) {
  timespec ts;
  ts.tv_sec = ms.count() / 1000;
  ts.tv_nsec = (ms.count() % 1000) * 1'000'000;
  nanosleep(&ts, nullptr);
}

#else
#error "Unsupported platform"
#endif
