
#include "LEDFunctions.hpp" 
void LED_PulseNTimes(ViSession instr, ViInt16 led_index,
                            ViInt32 time_between_pulses_ms,
                            ViInt32 pulse_width_ms, ViInt32 n_pulses,
                            ViInt16 brightness) {
  int led_brightnesses[6] = {0, 0, 0, 0, 0, 0};
  ViBoolean led_states[6] = {VI_FALSE, VI_FALSE, VI_FALSE,
                             VI_FALSE, VI_FALSE, VI_FALSE};
  led_brightnesses[led_index] = brightness;
  led_states[led_index] = VI_TRUE;
  for (int i = 0; i < n_pulses; i++) {
    TL6WL_setLED_HeadBrightness(instr, led_brightnesses[0], led_brightnesses[1],
                                led_brightnesses[2], led_brightnesses[3],
                                led_brightnesses[4], led_brightnesses[5]);
    TL6WL_setLED_HeadPowerStates(instr, led_states[0], led_states[1],
                                 led_states[2], led_states[3], led_states[4],
                                 led_states[5]);
    Sleep(pulse_width_ms);
    TL6WL_setLED_HeadPowerStates(instr, VI_FALSE, VI_FALSE, VI_FALSE, VI_FALSE,
                                 VI_FALSE, VI_FALSE);
    Sleep(time_between_pulses_ms);
  }
}