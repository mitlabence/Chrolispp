#define FIRMWARE_VERSION 4  // 1 is digital, 2 is PWM, 3 is MCP4725 code for this project <v2.0.0 (i.e. deprecated), 4 for version compatible with Chrolis++ v2.0.0. \
                       //Used to store in log file after recording
#include <Wire.h>
#include <Adafruit_MCP4725.h>  // install in Arduino IDE by Tools -> Manage Libraries... -> search for Adafruit_MCP4725, version 2.0.2 used here
Adafruit_MCP4725 dac;

// Command words for Arduino communication
constexpr uint8_t APPEND_STEP =
  10;  // Command word: append this step to the Arduino's internal queue. Expected response: CRC (should equal packet's CRC)
constexpr uint8_t REMOVE_LAST_STEP =
  20;                                        // Command word: pop previous step. Should return same byte + 1 (21) on
                                             // success
constexpr uint8_t RESET = 30;                // Command word: reset Arduino internal queue.
                                             // Should return same byte + 1 (31) on success
constexpr uint8_t EXECUTE = 40;              // Command word: start executing queued steps.
                                             // Should return same byte + 1 (41) on success
constexpr uint8_t VERSION_CHECK = 50;        // Command word: check Arduino firmware version. Should return FIRMWARE_VERSION
constexpr uint8_t LEGACY_CHECK = 60;         // Replaces legacy 6666 command + manually checking DAC levels: returns firmware version, blinks LEDs and moves DAC to max, to half, then to 0 again in a short time.
constexpr uint8_t CRC_MISMATCH_ERROR = 255;  // Error code returned by Arduino
                                             // if CRC mismatch occurs


// TODO: right now, LEGACY_CHECK is the character "<" (with No line ending setting obviously). Change command words for letter ascii codes! like append = a, delete = d, reset = r, execute = e, version check = v, legacy = l

#pragma pack(push, 1)
struct ArduinoDataPacket {
  uint8_t commandWord;
  uint32_t stepDuration;
  uint8_t isMicroseconds;
  uint16_t brightnessScaled;
  uint8_t crc;  // simple 8-bit checksum
};
#pragma pack(pop)

const size_t PACKET_SIZE = sizeof(ArduinoDataPacket);
const size_t PAYLOAD_SIZE = sizeof(ArduinoDataPacket) - 1;  // excluding commandWord
uint8_t computeCRC(const ArduinoDataPacket& packet) {
  // TODO: This is actually not CRC but checksum for now.Implement a CRC
  // variant. (This should be the same as in the Chrolispp source code at all times)
  const uint8_t* data = reinterpret_cast<const uint8_t*>(&packet);
  size_t length =
    sizeof(ArduinoDataPacket) - sizeof(uint8_t);  // exclude CRC itself
  uint8_t sum = 0;
  for (size_t i = 0; i < length; ++i) {
    sum ^= data[i];
  }
  return sum;
}

void blinkNTimes(short N) {
  if (N <= 0) { return; }
  // First N-1 times wait after turning off again
  for (int i = 0; i < N - 1; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
  // Do not wait at end of last one
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
}


const size_t MAX_QUEUE_SIZE = 64;
ArduinoDataPacket queue[MAX_QUEUE_SIZE];
size_t queueSize = 0;


uint16_t val = 0;

void setup() {
  Serial.begin(9600);
  while (Serial.available()) {
    Serial.read(); // discard incoming bytes
  }
  pinMode(LED_BUILTIN, OUTPUT);
  dac.begin(0x60);
  blinkNTimes(5);
}

void loop() {
  if (Serial.available()) {
    //uint8_t command = Serial.parseInt();
    uint8_t command = Serial.read();
    switch (command) {
      case APPEND_STEP:
        uint8_t buffer[PAYLOAD_SIZE];
        // Wait for full payload
        while (Serial.available() < PAYLOAD_SIZE)
          ;
        Serial.readBytes(reinterpret_cast<char*>(buffer), PAYLOAD_SIZE);
        ArduinoDataPacket pkt;
        pkt.commandWord = command;

        // Copy remaining fields from buffer
        memcpy(&pkt.stepDuration, buffer, PAYLOAD_SIZE);

        uint8_t crc_computed;
        crc_computed = computeCRC(pkt);
        if (pkt.crc == crc_computed) {
          if (queueSize < MAX_QUEUE_SIZE) {
            queue[queueSize++] = pkt;
          }
          Serial.write(pkt.crc);  // ACK
        } else {
          Serial.write(crc_computed);  // NACK or error echo
        }
        break;
      case REMOVE_LAST_STEP:
        if (queueSize > 0) queueSize--;
        Serial.write(REMOVE_LAST_STEP + 1);  // Expected response is same command word + 1
        break;
      case RESET:
        queueSize = 0;
        dac.setVoltage(0, false);
        Serial.write(RESET + 1);
        break;
      case EXECUTE:
        Serial.write(EXECUTE + 1);
        for (size_t i = 0; i < queueSize; ++i) {
          const ArduinoDataPacket& pkt = queue[i];
          unsigned long startTime = pkt.isMicroseconds ? micros() : millis();
          dac.setVoltage(pkt.brightnessScaled, false);
          unsigned long endTime = pkt.isMicroseconds ? micros() : millis();
          unsigned long elapsed = endTime - startTime;
          if (pkt.stepDuration > elapsed) {
            unsigned long remaining = pkt.stepDuration - elapsed;
            if (pkt.isMicroseconds) {
              delayMicroseconds(pkt.stepDuration);
            } else {
              delay(pkt.stepDuration);
            }
          }
        }
        queueSize = 0;  // clear after execution
        break;
      case VERSION_CHECK: // blink 2 times
        Serial.write(FIRMWARE_VERSION);
        blinkNTimes(2);
        break;
      case LEGACY_CHECK: // blink 3 times
        Serial.write(FIRMWARE_VERSION);
        dac.setVoltage(4095, false);
        blinkNTimes(1);
        delay(100);
        dac.setVoltage(2048, false);
        blinkNTimes(1);
        delay(100);
        dac.setVoltage(0, false);
        blinkNTimes(1);
        break;
    }
  }
}
