#include "ProtocolStep.hpp"

#include <cstring>  // Include for strcpy
#include <iostream>

// Constructor definition
ProtocolStep::ProtocolStep(ViUInt16 led_index, ViUInt32 pulse_width_ms,
                           ViUInt32 time_between_pulses_ms, ViUInt32 n_pulses,
                           ViUInt16 brightness) {
  if (brightness ==
      0) {  // Unify break notation: either pulse_width 0, or brightness = 0; in
            // latter case, pulse duration also adds to break. To avoid handling
            // this possibility in other places, unify from start. Thus break
            // always has 0 pulse_width_ms, and have 0 brightness. Also, ignore
            // n_pulses, make the break only once. To make it more obvious, set
            // n_pulses to 1 if break is detected.
    time_between_pulses_ms += pulse_width_ms;
    pulse_width_ms = 0;
    n_pulses = 1;
    led_index = 0;  // TODO: if break detected, make led_index invalid?
  } else if (pulse_width_ms == 0) {
    brightness = 0;
    n_pulses = 1;
    led_index = 0;
  }
  // If a gapless single pulse is wanted (i.e. with no trailing break), n_pulses
  // is taken as the multiplier (i.e. the total pulse duration is n_pulses *
  // pulse_width_ms). I.e. 2 pulses of 100 ms with no break is the same as 1
  // pulse of 200 ms.
  if (time_between_pulses_ms == 0) {
    pulse_width_ms *= n_pulses;
    n_pulses = 1;
  }
  this->led_index = led_index;
  this->pulse_width_ms = pulse_width_ms;
  this->time_between_pulses_ms = time_between_pulses_ms;
  this->brightness = brightness;
  this->n_pulses = n_pulses;
}

/*
 Check if the step is a single pulse with no trailing break (i.e. time between
 pulses = 0 ms). From the constructor, it is guaranteed that in this case
 n_pulses == 1.
*/
bool ProtocolStep::isGaplessSinglePulse() const {
  return (time_between_pulses_ms == 0);
}
/*
    After the unification of the break definition in the constructor, use this
    function to check whether the step is a break.*/
bool ProtocolStep::isBreak() const { return (brightness == 0); }
ViUInt32 ProtocolStep::getBreakDurationMs() const {
  if (isBreak()) {
    return time_between_pulses_ms;
  } else {
    throw std::logic_error(
        "getBreakDuration() called on non-break ProtocolStep.");
  }
}
void ProtocolStep::setBreakDuration(int break_duration_ms) {
  if (isBreak()) {
    time_between_pulses_ms = break_duration_ms;
  } else {
    throw std::logic_error(
        "setBreakDuration() called on non-break ProtocolStep.");
  }
}

void ProtocolStep::printStep() {
  // If pulse width is 0, it means a break
  if (pulse_width_ms == 0) {
    std::cout << "Break, duration: " << time_between_pulses_ms << " ms"
              << std::endl;
  } else if (brightness == 0) {
    std::cout << "Break, duration: " << pulse_width_ms + time_between_pulses_ms
              << " ms" << std::endl;
  } else {
    std::cout << "LED index: " << led_index << ", "
              << "Pulse width: " << pulse_width_ms << " ms, "
              << "Time between pulses: " << time_between_pulses_ms << " ms, "
              << "Number of pulses: " << n_pulses << ", "
              << "Brightness: " << brightness << std::endl;
  }
}

char* ProtocolStep::stepToChars() {
  // Calculate the required buffer size
  const int bufferSize =
      200;  // Adjust this size based on your actual requirements
  char* stepChars = new char[bufferSize];

  // Format the string into the buffer
  if (pulse_width_ms == 0) {
    std::snprintf(stepChars, bufferSize, "Break, duration: %d ms",
                  time_between_pulses_ms);
  } else if (brightness == 0) {
    std::snprintf(stepChars, bufferSize, "Break, duration: %d ms",
                  pulse_width_ms + time_between_pulses_ms);
  } else {
    std::snprintf(
        stepChars, bufferSize,
        "LED index: %d, Pulse width: %d ms, Time between pulses: %d ms, "
        "Number of pulses: %d, Brightness: %d",
        led_index, pulse_width_ms, time_between_pulses_ms, n_pulses,
        brightness);
  }
  return stepChars;
}

ViUInt32 ProtocolStep::getTotalDurationMs() const {
  if (isBreak()) {
    return time_between_pulses_ms;
  } else {
    return n_pulses * (pulse_width_ms + time_between_pulses_ms);
  }
}
