#ifndef LED_FUNCTIONS_HPP
#define LED_FUNCTIONS_HPP

#include <Windows.h>
#include "TL6WL.h"
#include "Logger.hpp"
void LED_PulseNTimes(ViSession instr, ViInt16 led_index, ViInt32 pulse_width_ms,
                            ViInt32 time_between_pulses_ms,
                             ViInt32 n_pulses,
                            ViInt16 brightness);
void LED_PulseNTimesWithArduino(ViSession instr, ViInt16 led_index,
                                ViInt32 pulse_width_ms,
                                ViInt32 time_between_pulses_ms,
                                ViInt32 n_pulses, ViInt16 brightness,
                                HANDLE h_Serial);
#endif  // LED_FUNCTIONS_HPP