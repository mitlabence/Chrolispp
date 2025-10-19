#pragma once
#include <atomic>
#include <condition_variable>
#include <queue>
#include "TL6WL.h"

struct ArduinoResources {
  std::queue<ViUInt16> arduinoMsgQueue;  // The queue for values to be written
                                         // (immediately) to Arduino.
  std::mutex arduinoMsgMutex;
  std::condition_variable arduinoMsgCV;
  std::atomic<bool> stopArduinoMsgThread{false};
};