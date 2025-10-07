#include "Timing.hpp"

#if defined(_WIN32)
#pragma comment(lib, "Winmm.lib")
#include <windows.h>

#include <iostream>

void Timing::precise_sleep_for(std::chrono::milliseconds ms) {
  // Request 1 ms timer resolution on Windows
  DWORD dw_ms = static_cast<DWORD>(ms.count());
  //std::cout << "Sleeping for " << dw_ms << " ms" << std::endl;
  timeBeginPeriod(1);
  Sleep(dw_ms);
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
