#include "ProtocolStep.hpp"
#include <iostream>
#include <cstring> // Include for strcpy

// Constructor definition
ProtocolStep::ProtocolStep(ViInt16 led_index, ViInt32 pulse_width_ms,
                           ViInt32 time_between_pulses_ms,
                           ViInt32 n_pulses,
                           ViInt16 brightness)
    : led_index(led_index),
      pulse_width_ms(pulse_width_ms),
      time_between_pulses_ms(time_between_pulses_ms),
      n_pulses(n_pulses),
      brightness(brightness) {
}

void ProtocolStep::printStep() {
  // If pulse width is 0, it means a break
  if (pulse_width_ms == 0) {
    std::cout << "Break, duration: " << time_between_pulses_ms << " ms"
              << std::endl;
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
