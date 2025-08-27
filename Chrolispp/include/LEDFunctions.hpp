#ifndef LED_FUNCTIONS_HPP
#define LED_FUNCTIONS_HPP

#include <Windows.h>

#include "Logger.hpp"
#include "TL6WL.h"

bool LED_HandleBreak(ViUInt32 time_between_pulses_ms,
                     ViUInt32 brightness);
bool LED_ValidateLEDIndex(ViUInt16 led_index);
bool LED_ValidateParams(ViUInt16 led_index, ViUInt32 n_pulses,
                        ViUInt16& brightness);
bool LED_ValidateBrightness(ViUInt16& brightness);
ViStatus LED_DoSequence(ViSession instr, ViUInt16 led_index,
                        ViUInt32 pulse_width_ms,
                        ViUInt32 time_between_pulses_ms, ViUInt32 n_pulses,
                        ViInt16 brightness, bool use_bob);
void LED_PulseNTimes(ViSession instr, ViUInt16 led_index,
                     ViUInt32 pulse_width_ms, ViUInt32 time_between_pulses_ms,
                     ViUInt32 n_pulses, ViUInt16& brightness, bool use_bob);
void LED_PulseNTimesWithArduino(ViSession instr, ViUInt16 led_index,
                                ViUInt32 pulse_width_ms,
                                ViUInt32 time_between_pulses_ms,
                                ViUInt32 n_pulses, ViUInt16& brightness,
                                HANDLE h_Serial, int dac_resolution_bits,
                                bool use_bob);

std::string readBoxStatusWarnings(ViUInt32 boxStatus);
class led_machine_error : public std::exception {
 public:
  explicit led_machine_error(const std::string& message) : message_(message) {}

  virtual const char* what() const noexcept override {
    return message_.c_str();
  }

 private:
  std::string message_;
};
#endif  // LED_FUNCTIONS_HPP