
#include "LEDFunctions.hpp" 
#include "Logger.hpp"
#include <iostream>
const char dataLightOn[] = "1";
const char dataLightOff[] = "0";

void LED_PulseNTimes(ViSession instr, ViInt16 led_index, ViInt32 pulse_width_ms,
                            ViInt32 time_between_pulses_ms,
                             ViInt32 n_pulses,
                            ViInt16 brightness) {
  if (pulse_width_ms == 0) {
    Sleep(time_between_pulses_ms);
    return;
  }
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

void LED_PulseNTimesWithArduino(ViSession instr, ViInt16 led_index, ViInt32 pulse_width_ms,
                     ViInt32 time_between_pulses_ms, ViInt32 n_pulses,
                     ViInt16 brightness, HANDLE h_Serial) {
  if (pulse_width_ms == 0) {
    Sleep(time_between_pulses_ms);
    return;
  }
  DWORD dwBytesWritten;
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
    // Try to write to arduino
    if (!WriteFile(h_Serial, dataLightOn, sizeof(dataLightOn), &dwBytesWritten,
                   NULL)) {
      std::cout << "Error writing to serial port LED on" << std::endl;
    }
    Sleep(pulse_width_ms);
    TL6WL_setLED_HeadPowerStates(instr, VI_FALSE, VI_FALSE, VI_FALSE, VI_FALSE,
                                 VI_FALSE, VI_FALSE);
    if (!WriteFile(h_Serial, dataLightOff, sizeof(dataLightOn), &dwBytesWritten,
                   NULL)) {
      std::cout << "Error writing to serial port LED off" << std::endl;
    }
    Sleep(time_between_pulses_ms);
  }
}