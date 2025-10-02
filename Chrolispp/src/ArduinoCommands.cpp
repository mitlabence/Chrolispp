#include "ArduinoCommands.hpp"

#include "Utils.hpp"
int scaleBrightnessToArduino(HANDLE h_Serial, ViUInt16& brightness,
                             int dac_resolution_bits) {
  if (dac_resolution_bits >
      16) {  // 17-bit resolution would mean a max number with 6 digits (buffer
             // size would be 7). Draw a line of support here.
    throw std::out_of_range(
        "DAC resolution too high: " + std::to_string(dac_resolution_bits) +
        ". Maximum supported resolution is 16-bit.");
  }

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
  return brightness_remapped;
}

void sendRemappedBrightnessToArduino(HANDLE h_Serial, int brightness_remapped) {
  /*
  Send a brightness value to the Arduino over serial.
  */
  const size_t bufferSize =
      6;  // buffer size for converting the digital value to a string
  DWORD dwBytesWritten;
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
}

int sendBrightnessToArduino(HANDLE h_Serial, ViUInt16& brightness,
                            int dac_resolution_bits) {
  int brightness_remapped =
      scaleBrightnessToArduino(h_Serial, brightness, dac_resolution_bits);
  sendRemappedBrightnessToArduino(h_Serial, brightness_remapped);
  return brightness_remapped;
}