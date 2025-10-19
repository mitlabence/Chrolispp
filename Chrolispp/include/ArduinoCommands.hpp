#ifndef ARDUINO_COMMANDS_HPP
#define ARDUINO_COMMANDS_HPP

#include <Windows.h>

#include <iostream>
#include <stdexcept>
#include <string>

#include "TL6WL.h"
int scaleBrightnessToArduino(HANDLE h_Serial, ViUInt16& brightness,
                             int dac_resolution_bits);
void sendRemappedBrightnessToArduino(HANDLE h_Serial, int brightness_remapped);
int sendBrightnessToArduino(HANDLE h_Serial, ViUInt16& brightness,
                            int dac_resolution_bits);
#endif  // ARDUINO_COMMANDS_HPP