
#include "LEDFunctions.hpp" 
#include "Logger.hpp"
#include "Utils.hpp"
#include <iostream>
#include <exception>
#include <string>

const char DATA_LIGHT_OFF[] = "1";

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
                     ViInt16 brightness, HANDLE h_Serial, int dac_resolution_bits) {
    /*
    dac_resolution: if set to 0, no communication with an Arduino board is attempted.
    Otherwise, brightness will be remapped to fit in the specified resolution (dac_resolution=8 means 8-bit etc.).
    If set to 1, 
    */
  if (pulse_width_ms == 0) {
    Sleep(time_between_pulses_ms);
    return;
  }
  if (dac_resolution_bits > 16) { // 17-bit resolution would mean a max number with 6 digits (buffer size would be 7). Draw a line of support here.
      throw std::out_of_range("DAC resolution too high: " + std::to_string(dac_resolution_bits) + ". Maximum supported resolution is 16-bit.");
  }
  const size_t bufferSize = 6; // buffer size for converting the digital value to a string
  DWORD dwBytesWritten;
  int led_brightnesses[6] = {0, 0, 0, 0, 0, 0};
  ViBoolean led_states[6] = {VI_FALSE, VI_FALSE, VI_FALSE,
                             VI_FALSE, VI_FALSE, VI_FALSE};
  if (brightness > 1000) { // limit is 1000 = 100.0%
      led_brightnesses[led_index] = 1000;
  }
  else {
      led_brightnesses[led_index] = brightness;
  }
  // map int16 to 0-255 for arduino 8-bit resolution output
  int brightness_remapped = -1; // if stays negative, do not send.
  if (dac_resolution_bits > 2) {
	  int dac_resolution = pow(2, dac_resolution_bits) - 1;
      brightness_remapped = static_cast<int>(brightness / 1000.0 * dac_resolution) + 1; // add 1 because range starts from 1 in Arduino (0 is reserved for timeout in Serial.parseInt())
  }
  else if (dac_resolution_bits == 1) {
      // digital output, but cannot send 0 or 1 for "on" signal. Send 2 instead.
      if (brightness > 0) {
          brightness_remapped = 2;
      }
      else {
		  brightness_remapped = 1; // 1 is the off signal for Arduino (0 is reserved for timeout).
      }
  }
  char message[bufferSize];
  try {
      intToCharArray(brightness_remapped, message, bufferSize);
  }
  catch (const std::exception& e) {
	  std::cerr << e.what() << std::endl;
  }

  led_states[led_index] = VI_TRUE;
  for (int i = 0; i < n_pulses; i++) {
    TL6WL_setLED_HeadBrightness(instr, led_brightnesses[0], led_brightnesses[1],
                                led_brightnesses[2], led_brightnesses[3],
                                led_brightnesses[4], led_brightnesses[5]);
    TL6WL_setLED_HeadPowerStates(instr, led_states[0], led_states[1],
                                 led_states[2], led_states[3], led_states[4],
                                 led_states[5]);
    // Try to write to arduino
    if (brightness_remapped >= 0) {
        if (!WriteFile(h_Serial, &message, sizeof(message), &dwBytesWritten,
            NULL)) {
            std::cout << "Error writing to serial port LED on" << std::endl;
        }
    }
    Sleep(pulse_width_ms);
    TL6WL_setLED_HeadPowerStates(instr, VI_FALSE, VI_FALSE, VI_FALSE, VI_FALSE,
                                 VI_FALSE, VI_FALSE);
    if (brightness_remapped >= 0) {
        if (!WriteFile(h_Serial, DATA_LIGHT_OFF, sizeof(DATA_LIGHT_OFF), &dwBytesWritten,
            NULL)) {
            std::cout << "Error writing to serial port LED off" << std::endl;
        }
    }
    Sleep(time_between_pulses_ms);
  }
}


std::string readBoxStatusWarnings(ViUInt32 boxStatus) {
  int bit0, bit1, bit2, bit3, bit4, bit5, bit6;
  if (bit0 = (boxStatus & 0x01)) {
    return "Box is open";
  } else if (bit1 = (boxStatus & 0x02)) {
    return "LLG not connected";
  }
  else if (bit2 = (boxStatus & 0x04)) {
    return "Interlock is open";
  }

  else if (bit3 = (boxStatus & 0x08)) {
    return "Using default adjustment";
  }
  else if (bit4 = (boxStatus & 0x10)) {
    return "Box overheated";
  }

  else if (bit5 = (boxStatus & 0x20)) {
    return "LED overheated";
  }
  else if (bit6 = (boxStatus & 0x40)) {
    return "Box setup invalid";
  }
  // If LED or Box overheated, abort protocol
  if (bit4 || bit5) {
    throw led_machine_error("Box or LED overheated. Protocol aborted.");
  }
  // If everything all right, do not return any warnings
  return "";
}


