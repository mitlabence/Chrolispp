#ifndef ARDUINO_COMMANDS_HPP
#define ARDUINO_COMMANDS_HPP

#include <Windows.h>

#include <iostream>
#include <stdexcept>
#include <string>

#include "TL6WL.h"

// Command words for Arduino communication
constexpr uint8_t APPEND_STEP =
    10;  // Command word: append this step to the Arduino's internal queue.
         // Expected response: CRC (should equal packet's CRC)
constexpr uint8_t REMOVE_LAST_STEP =
    20;  // Command word: pop previous step. Should return same byte + 1 (21) on
         // success
constexpr uint8_t RESET = 30;    // Command word: reset Arduino internal queue.
                                 // Should return same byte + 1 (31) on success
constexpr uint8_t EXECUTE = 40;  // Command word: start executing queued steps.
                                 // Should return same byte + 1 (41) on success
constexpr uint8_t VERSION_CHECK =
    50;  // Command word: check if Arduino is running properly. Should return
         // firmware version (e.g. 4, the first compatible version)
constexpr uint8_t LEGACY_CHECK =
    60;  // Replaces legacy 6666 command + manually checking DAC levels: returns
         // firmware version, blinks LEDs and moves DAC to max, to half, then to
         // 0 again in a short time.

struct ArduinoDataPacket {
  uint8_t commandWord;  // e.g., CMD_SET_BRIGHTNESS_AND_DURATION
  uint32_t stepDuration;
  uint8_t isMicroseconds;     // 0 if milliseconds, 1 if microseconds
  uint16_t brightnessScaled;  // Should be the remapped value (to the DAC range,
                              // see scaleBrightnessToArduino)
  uint8_t crc;                // CRC for error checking
};

ArduinoDataPacket createStepDataPacket(ViUInt16& brightness,
                                       uint32_t stepDuration,
                                       bool isMicroseconds,
                                       int dac_resolution_bits);

ViUInt16 scaleBrightnessToArduino(ViUInt16& brightness,
                                  int dac_resolution_bits);
uint8_t sendCommandToArduino(HANDLE h_Serial, uint8_t command);
uint16_t sendDataPacketToArduino(HANDLE h_Serial, ArduinoDataPacket& packet,
                                 int dac_resolution_bits);
uint8_t computeCRC(const ArduinoDataPacket& packet);
#endif  // ARDUINO_COMMANDS_HPP