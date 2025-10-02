
#include "LEDFunctions.hpp"

#include <exception>
#include <iostream>
#include <string>

#include "Logger.hpp"
#include "Utils.hpp"

const char DATA_LIGHT_OFF[] = "1";

bool LED_ValidateLEDIndex(ViUInt16 led_index) {
  /* Validate LED index. Returns true if valid, false if invalid.
  Valid indices are 0-5 (6 LEDs).
*/
  if (led_index < 0 || led_index > 5) {
    return false;
  }
  return true;
}

bool LED_HandleBreak(ViUInt32 time_between_pulses_ms, ViUInt32 brightness) {
  // a break is defined by 0 brightness. Then the time_between_pulses_ms is
  // considered. Other logical definitions of break should be unified in the
  // ProtocolStep constructor, bringing them to this common format. (for
  // example, setting brightness = 0 and pulse_width_ms != 0 should mean a break
  // of pulse_width_ms + time_between_pulses_ms; this is handled in the
  // constructor).
  if (brightness == 0) {
    Sleep(time_between_pulses_ms);
    return true;
  }
  return false;
}

bool LED_ValidateBrightness(ViUInt16& brightness) {
  /* Validate brightness value. If out of range, bring into range (negative
   * values to 0 [i.e. break], >1000 values to 1000 [i.e. 100.0%]). Returns true
   * if brightness was in range, false if it was out of range and had to be
   * changed.
   */
  if (brightness > 1000) {
    std::cout << "Brightness > 100.0%: " << std::to_string(brightness)
              << "... Using value 1000 (100.0%)" << std::endl;
    brightness = 1000;
    return false;
  }
  return true;
}

bool LED_ValidateParams(ViUInt16 led_index, ViUInt32 n_pulses,
                        ViUInt16& brightness) {
  // DEPRECATED. Use ProtocolPlanner::validateStep instead.
  /* Validate parameters for LED pulsing functions. Returns true if protocol
    should proceed, false if no action should be taken. Raises exception if LED
    index is invalid. If necessary, brings LED power into valid range (negative
    values to 0 [i.e. break], >1000 values to 1000 [i.e. 100.0%]). Checks the
    following:
    1. If n_pulses is 0, return false (no action).
    2. If led_index is out of range (not 0-5), raise out_of_range exception.
    3. If brightness is out of range (not 0-1000), warn user and bring into
    range, then proceed with check.
    4. Otherwise, return true (proceed with protocol).
  */
  if (n_pulses == 0) {  // if 0 pulses (and not a break, i.e. pulse_width_ms !=
                        // 0), do not do anything.
    std::cout           // TODO: handle this more elegantly, pre-check input?
        << "0 pulses and pulse duration != 0, not a valid step..." << std::endl;
    return false;
  }
  if (!LED_ValidateLEDIndex(led_index)) {
    throw std::out_of_range("LED index invalid: " + std::to_string(led_index));
  }
  LED_ValidateBrightness(brightness);
  return true;
}

bool LED_isBreak(ViUInt32 pulse_width_ms, ViUInt32 time_between_pulses_ms,
                 ViUInt32 brightness) {
  // DEPRECATED, use isBreak() in ProtocolPlanner.cpp instead.
  /* Check if the specified brightness indicates a break (0 brightness).
  Returns true if it is a break, false otherwise.
  */
  if (brightness == 0) {
    return true;
  }
  return false;
}

/*
led_index: 0-5 for the 6 LEDs.
brightness: the integer format brightness value for the LED specified to
    one decimal (i.e. 0 is 0.0%, 10 is 1.0%, 525 is 52.5%, 1000 is 100.0%).
use_bob: whether to use breakout board to output the same signal on a TTL
output. As the LED and the BOB signal use same internal timer, they should be
synchronized, but cannot be guaranteed by this software.
*/
ViStatus LED_ConfigureSingleStep(ViSession instr, ViUInt16 led_index,
                                 ViUInt32 start_delay_us,
                                 ViUInt32 pulse_width_ms,
                                 ViUInt32 time_between_pulses_ms,
                                 ViUInt32 n_pulses, ViUInt16 brightness,
                                 bool use_bob) {
  ViStatus err;  // TODO: add error handling
  err = TL6WL_TU_AddGeneratedSelfRunningSignal(
      instr, led_index + 1, VI_FALSE, start_delay_us, pulse_width_ms * 1000,
      time_between_pulses_ms * 1000, n_pulses);
  if (use_bob) {
    // Also set up corresponding TTL output channel for getting output timing
    // signal (digital, i.e. no LED brightness info!) channels 7-12 are the
    // output channels
    // led_index + 1 + 6
    err = TL6WL_TU_AddGeneratedSelfRunningSignal(
        instr, 6 + led_index + 1, VI_FALSE, start_delay_us,
        pulse_width_ms * 1000, time_between_pulses_ms * 1000, n_pulses);
  }
  return err;
}

/*
Batch configuration of multiple LED steps. One batch is a sequence of steps that
can be pre-programmed in the LED machine simultaneously, and does not include a
break (that is, each LED index may occur once or not occur). Each vector should
have the same length, and each led index different (the definition of a batch),
except if it is a break (0 brightness). The steps will be configured in the
order of the vectors, with no (intended) delay between them (i.e. the start time
of step i+1 is the end time of step i). The function resets any previous
configuration on the device.
*/
ViStatus LED_ConfigureBatch(ViSession instr, std::vector<ViUInt16>& led_indices,
                            std::vector<ViUInt32>& pulse_widths_ms,
                            std::vector<ViUInt32>& times_between_pulses_ms,
                            std::vector<ViUInt32>& ns_pulses,
                            std::vector<ViUInt16>& brightnesses,
                            std::vector<bool>& use_bob) {
  char leds_present =
      0b000000;  // bitmask of which LEDs are already used in this batch
  size_t size = led_indices.size();  // assuming all vectors have the same size
  ViStatus err;
  err = TL6WL_TU_ResetSequence(instr);
  int total_duration_us = 0;
  // TODO: add error handling
  for (size_t i = 0; i < size; ++i) {
    ViUInt16 led_index = led_indices[i];
    ViUInt32 pulse_width_ms = pulse_widths_ms[i];
    ViUInt32 time_between_pulses_ms = times_between_pulses_ms[i];
    ViUInt32 n_pulses = ns_pulses[i];
    ViUInt16 brightness = brightnesses[i];
    bool bob = use_bob[i];
    LED_ValidateParams(led_index, n_pulses, brightness);
    LED_ValidateBrightness(brightness);

    err = LED_ConfigureSingleStep(instr, led_index, total_duration_us,
                                  pulse_width_ms, time_between_pulses_ms,
                                  n_pulses, brightness, bob);
    if (err != VI_SUCCESS) {
      printf(" LED_ConfigureSingleStep  :\n    Error Code = %#.8lX\n", err);
    }
    total_duration_us +=
        n_pulses * (pulse_width_ms + time_between_pulses_ms) * 1000;
  }
  return err;
}

ViStatus LED_DoSequence(ViSession instr, ViUInt16 led_index,
                        ViUInt32 pulse_width_ms,
                        ViUInt32 time_between_pulses_ms, ViUInt32 n_pulses,
                        ViInt16 brightness, bool use_bob) {
  /* Perform the LED sequence WITHOUT performing any parameter validation (use
   * LED_PulseNTimes or LED_PulseNTimesWithArduino instead).
   * Parameters:
   *    instr: ViSession handle to the TL6WL device.
   *    led_index: 0-5 for the 6 LEDs.
   *    pulse_width_ms: LED on time within one cycle in milliseconds.
   *        time_between_pulses_ms: Time between the end of one pulse and the
   *        start of the next (i.e. the LED off time within one cycle) in
   *        milliseconds.
   *    n_pulses: the number of pulses (cycles). If 0, no action is taken.
   *    brightness: the integer format brightness value for the LED specified to
   *        one decimal (i.e. 0 is 0.0%, 10 is 1.0%, 525 is 52.5%, 1000 is
   *        100.0%).
   *    use_bob: if true, use the breakout board to send timing signals. If
   *        false, no timing signals are sent.
   */
  ViStatus err;
  int led_brightnesses[6] = {0, 0, 0, 0, 0, 0};
  ViBoolean led_states[6] = {VI_FALSE, VI_FALSE, VI_FALSE,
                             VI_FALSE, VI_FALSE, VI_FALSE};
  led_brightnesses[led_index] = brightness;
  led_states[led_index] = VI_TRUE;
  err = TL6WL_TU_ResetSequence(instr);
  TL6WL_setLED_HeadBrightness(instr, led_brightnesses[0], led_brightnesses[1],
                              led_brightnesses[2], led_brightnesses[3],
                              led_brightnesses[4], led_brightnesses[5]);
  // led_index is 0-indexing (0-5), need 1-6; function takes us values, so ms
  // * 1000
  // TODO: add error handling, see (example code)
  // https://github.com/Thorlabs/Light_Sources_Examples/blob/main/C%2B%2B/C_Chrolis/CHROLIS_CSample/CHROLIS_TimingUnit_CppSample.cpp
  TL6WL_TU_AddGeneratedSelfRunningSignal(
      instr, led_index + 1, VI_FALSE, 0, pulse_width_ms * 1000,
      time_between_pulses_ms * 1000, n_pulses);
  if (use_bob) {
    // Also set up corresponding TTL output channel for getting output timing
    // signal (digital, i.e. no LED brightness info!) channels 7-12 are the
    // output channels
    // led_index + 1 + 6
    TL6WL_TU_AddGeneratedSelfRunningSignal(
        instr, 7, VI_FALSE, 0, pulse_width_ms * 1000,
        time_between_pulses_ms * 1000, n_pulses);
  }
  TL6WL_setLED_HeadPowerStates(instr, led_states[0], led_states[1],
                               led_states[2], led_states[3], led_states[4],
                               led_states[5]);
  err = TL6WL_TU_StartStopGeneratorOutput_TU(instr, true);
  Sleep(n_pulses *
        (pulse_width_ms +
         time_between_pulses_ms));  // TODO: more sophisticated waiting, maybe
                                    // as callback (something like await)?
  return err;
}

void LED_PulseNTimes(ViSession instr, ViUInt16 led_index,
                     ViUInt32 pulse_width_ms, ViUInt32 time_between_pulses_ms,
                     ViUInt32 n_pulses, ViUInt16& brightness, bool use_bob) {
  /* Pulse the specified LED n times with the given parameters. One such
   * sequence of N pulses consists of [LED on, LED off] N times. The value of
   * brightness may be overwritten if it is out of range.
   * Parameters:
   *    instr: ViSession handle to the TL6WL device.
   *    led_index: 0-5 for the 6 LEDs.
   *    pulse_width_ms: LED on time within one cycle in milliseconds.
   *        time_between_pulses_ms: Time between the end of one pulse and the
   *        start of the next (i.e. the LED off time within one cycle) in
   *        milliseconds.
   *    n_pulses: the number of pulses (cycles). If 0, no action is taken.
   *    brightness: the integer format brightness value for the LED specified to
   *        one decimal (i.e. 0 is 0.0%, 10 is 1.0%, 525 is 52.5%, 1000 is
   *        100.0%).
   *    use_bob: if true, use the breakout board to send timing signals. If
   *        false, no timing signals are sent.
   */
  // TODO: Add error handling
  ViStatus err;
  // Detect if this is a break (0 brightness or 0 pulse width) and handle it
  if (LED_HandleBreak(time_between_pulses_ms, brightness)) {
    return;
  }
  try {
    bool isValid = LED_ValidateParams(led_index, n_pulses, brightness);
    if (!isValid) {
      return;
    }
  } catch (const std::out_of_range& e) {
    // TODO: handle the error or re-raise?
    // std::cerr << e.what() << std::endl;
    std::cout << "LED_ValidateParams: " << e.what() << std::endl;
    return;
  }
  err = LED_DoSequence(instr, led_index, pulse_width_ms, time_between_pulses_ms,
                       n_pulses, brightness, use_bob);
  if (VI_SUCCESS != err) {
    printf(" LED_DoSequenceWithBOB  :\n    Error Code = %#.8lX\n", err);
  } else {
    printf("Successful LED sequence\n");
  }
}

void LED_PulseNTimesWithArduino(ViSession instr, ViUInt16 led_index,
                                ViUInt32 pulse_width_ms,
                                ViUInt32 time_between_pulses_ms,
                                ViUInt32 n_pulses, ViUInt16& brightness,
                                HANDLE h_Serial, int dac_resolution_bits,
                                bool use_bob) {
  /*
  dac_resolution: if set to 0, no communication with an Arduino board is
  attempted. Otherwise, brightness will be remapped to fit in the specified
  resolution (dac_resolution=8 means 8-bit etc.). If set to 1,
  */

  // TODO: Add error handling
  ViStatus err;
  // Detect if this is a break (0 brightness or 0 pulse width) and handle it
  if (LED_HandleBreak(time_between_pulses_ms, brightness)) {
    return;
  }

  try {
    bool isValid = LED_ValidateParams(led_index, n_pulses, brightness);
    if (!isValid) {
      return;
    }
  } catch (const std::out_of_range& e) {
    // TODO: handle the error or re-raise?
    // std::cerr << e.what() << std::endl;
    std::cout << "LED_ValidateParams: " << e.what() << std::endl;
    return;
  }
  if (dac_resolution_bits >
      16) {  // 17-bit resolution would mean a max number with 6 digits (buffer
             // size would be 7). Draw a line of support here.
    throw std::out_of_range(
        "DAC resolution too high: " + std::to_string(dac_resolution_bits) +
        ". Maximum supported resolution is 16-bit.");
  }
  const size_t bufferSize =
      6;  // buffer size for converting the digital value to a string
  DWORD dwBytesWritten;

  // map int16 to 0-255 for arduino 8-bit resolution output
  int brightness_remapped = -1;  // if stays negative, do not send.
  if (dac_resolution_bits > 2) {
    int dac_resolution = pow(2, dac_resolution_bits) - 1;
    brightness_remapped =
        static_cast<int>(brightness / 1000.0 * dac_resolution) +
        1;  // add 1 because range starts from 1 in Arduino (0 is reserved for
            // timeout in Serial.parseInt())
  } else if (dac_resolution_bits == 1) {
    // digital output, but cannot send 0 or 1 for "on" signal. Send 2 instead.
    if (brightness > 0) {
      brightness_remapped = 2;
    } else {
      brightness_remapped =
          1;  // 1 is the off signal for Arduino (0 is reserved for timeout).
    }
  }
  char message[bufferSize];
  try {
    intToCharArray(brightness_remapped, message, bufferSize);
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  if (brightness_remapped < 0) {
    std::cout << "DAC resolution not set, not sending command to Arduino."
              << std::endl;
  }

  // Try to write to arduino
  if (brightness_remapped >= 0) {
    if (!WriteFile(h_Serial, &message, sizeof(message), &dwBytesWritten,
                   NULL)) {
      std::cout << "Error writing to serial port LED on" << std::endl;
    }
  }

  err = LED_DoSequence(instr, led_index, pulse_width_ms, time_between_pulses_ms,
                       n_pulses, brightness, use_bob);
  // Write the "off" command to the Arduino
  if (brightness_remapped >= 0) {
    if (!WriteFile(h_Serial, DATA_LIGHT_OFF, sizeof(DATA_LIGHT_OFF),
                   &dwBytesWritten, NULL)) {
      std::cout << "Error writing to serial port LED off" << std::endl;
    }
  }

  if (VI_SUCCESS != err) {
    printf(" LED_DoSequenceWithBOB  :\n    Error Code = %#.8lX\n", err);
  } else {
    printf("Successful LED sequence\n");
  }
}

std::string readBoxStatusWarnings(ViUInt32 boxStatus) {
  int bit0, bit1, bit2, bit3, bit4, bit5, bit6;
  if (bit0 = (boxStatus & 0x01)) {
    return "Box is open";
  } else if (bit1 = (boxStatus & 0x02)) {
    return "LLG not connected";
  } else if (bit2 = (boxStatus & 0x04)) {
    return "Interlock is open";
  }

  else if (bit3 = (boxStatus & 0x08)) {
    return "Using default adjustment";
  } else if (bit4 = (boxStatus & 0x10)) {
    return "Box overheated";
  }

  else if (bit5 = (boxStatus & 0x20)) {
    return "LED overheated";
  } else if (bit6 = (boxStatus & 0x40)) {
    return "Box setup invalid";
  }
  // If LED or Box overheated, abort protocol
  if (bit4 || bit5) {
    throw led_machine_error("Box or LED overheated. Protocol aborted.");
  }
  // If everything all right, do not return any warnings
  return "";
}


