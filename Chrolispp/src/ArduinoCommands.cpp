#include "ArduinoCommands.hpp"

#include "COMFunctions.hpp"
#include "Utils.hpp"

ViUInt16 scaleBrightnessToArduino(ViUInt16& brightness,
                                  int dac_resolution_bits) {
  if (dac_resolution_bits >
      16) {  // 17-bit resolution would mean a max number with 6 digits (buffer
             // size would be 7). Draw a line of support here.
    throw std::out_of_range(
        "DAC resolution too high: " + std::to_string(dac_resolution_bits) +
        ". Maximum supported resolution is 16-bit.");
  }

  // map int16 to 0-255 for arduino 8-bit resolution output
  ViUInt16 brightness_remapped = 0;
  if (dac_resolution_bits > 2) {
    int dac_resolution = pow(2, dac_resolution_bits) - 1;
    brightness_remapped =
        static_cast<ViUInt16>(brightness / 1000.0 * dac_resolution) +
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

/* Create data packet to send to Arduino with command word APPEND_STEP
 */
ArduinoDataPacket createStepDataPacket(ViUInt16& brightness,
                                       uint32_t stepDuration,
                                       bool isMicroseconds,
                                       int dac_resolution_bits) {
  ArduinoDataPacket packet;
  packet.commandWord = APPEND_STEP;
  packet.stepDuration = stepDuration;
  packet.isMicroseconds = isMicroseconds ? 1 : 0;
  packet.brightnessScaled = static_cast<uint16_t>(
      scaleBrightnessToArduino(brightness, dac_resolution_bits));
  packet.crc = computeCRC(packet);
  return packet;
}

/* Send data packet to Arduino. The response (CRC) is returned.
 */
uint16_t sendDataPacketToArduino(HANDLE h_Serial, ArduinoDataPacket& packet,
                                 int dac_resolution_bits) {
  DWORD bytesWritten;
  DWORD bytesRead;
  uint8_t crcResponse = 0;
  try {
    BOOL success = WriteFile(h_Serial, reinterpret_cast<uint8_t*>(&packet),
                             sizeof(packet), &bytesWritten,
                             nullptr  // synchronous write
    );
    if (!success || bytesWritten != sizeof(packet)) {
      throw std::runtime_error("WriteFile failed or incomplete");
    }
    // Read the CRC response (1 byte)
    success = ReadFile(h_Serial, &crcResponse, 1, &bytesRead, nullptr);

    if (!success || bytesRead != 1) {
      throw std::runtime_error("Failed to read CRC response from Arduino");
    }

    return crcResponse;
  } catch (const std::exception& e) {
    std::cerr << "Error sending data packet to Arduino: " << e.what()
              << std::endl;
    // raise error
    throw std::runtime_error("Error sending data packet to Arduino: " +
                             std::string(e.what()));
  }
}

uint8_t sendCommandToArduinoOld(HANDLE h_Serial, uint8_t command) {
  char message[5];
  intToCharArray(command, message, sizeof(message));
  writeMessage(h_Serial, message, sizeof(message));
  char* response = readMessage(h_Serial, 1);
  return static_cast<uint8_t>(response[0]);
}

/*
Send single-byte command to Arduino.
*/
uint8_t sendCommandToArduino(HANDLE h_Serial, uint8_t command) {
  if (command == APPEND_STEP) {
    throw std::invalid_argument(
        "sendCommand: APPEND_STEP is not a single-byte command.");
  }
  DWORD bytesWritten;
  bool success = WriteFile(h_Serial, &command, 1, &bytesWritten, nullptr) &&
                 bytesWritten == 1;
  if (!success || bytesWritten != 1) {
    throw std::runtime_error("Failed to write command to Arduino");
  }
  // Read response byte
  uint8_t response = 0;
  DWORD bytesRead;
  success =
      ReadFile(h_Serial, &response, 1, &bytesRead, nullptr) && bytesRead == 1;
  if (!success || bytesRead != 1) {
    throw std::runtime_error("Failed to read response from Arduino");
  }
  return response;
}

uint8_t computeCRC(const ArduinoDataPacket& packet) {
  // TODO: This is actually not CRC but checksum for now.Implement a CRC
  // variant. (This should be the same as in the Chrolispp source code at all
  // times)
  const uint8_t* data = reinterpret_cast<const uint8_t*>(&packet);
  size_t length =
      sizeof(ArduinoDataPacket) - sizeof(uint8_t);  // exclude CRC itself
  uint8_t sum = 0;
  for (size_t i = 0; i < length; ++i) {
    sum ^= data[i];
  }
  return sum;
}