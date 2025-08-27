#include "ProtocolStep.hpp"

#include <cstring>  // Include for strcpy
#include <iostream>

// Constructor definition
ProtocolStep::ProtocolStep(ViInt16 led_index, ViInt32 pulse_width_ms,
                           ViInt32 time_between_pulses_ms, ViInt32 n_pulses,
                           ViUInt16 brightness)
    {
    // TODO: if break detected, make led_index invalid?
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
    led_index = 0;
  } else if (pulse_width_ms == 0) {
    brightness = 0;
    n_pulses = 1;
    led_index = 0;
  }
  this->led_index = led_index;
  this->pulse_width_ms = pulse_width_ms;
  this->time_between_pulses_ms = time_between_pulses_ms;
  this->brightness = brightness;
  this->n_pulses = n_pulses;
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
