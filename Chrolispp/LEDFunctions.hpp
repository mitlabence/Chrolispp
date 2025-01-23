#include <Windows.h>
#include "TL6WL.h"
void LED_PulseNTimes(ViSession instr, ViInt16 led_index,
                            ViInt32 time_between_pulses_ms,
                            ViInt32 pulse_width_ms, ViInt32 n_pulses,
                            ViInt16 brightness);