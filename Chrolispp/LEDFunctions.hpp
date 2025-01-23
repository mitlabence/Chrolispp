#include <Windows.h>
#include "TL6WL.h"
void LED_PulseNTimes(ViSession instr, ViInt8 led_index,
                            ViInt16 time_between_pulses_ms,
                            ViInt16 pulse_width_ms, ViInt32 n_pulses,
                            ViInt16 brightness);