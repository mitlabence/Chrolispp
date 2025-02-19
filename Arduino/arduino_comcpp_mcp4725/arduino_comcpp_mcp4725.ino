#define LEDPIN 13
#define FIRMWARE_ID 3  // 1 is digital, 2 is PWM, 3 is MCP4725 code for this project. Used to store in log file after recording
#include <Wire.h>
#include <Adafruit_MCP4725.h> // install in Arduino IDE by Tools -> Manage Libraries... -> search for Adafruit_MCP4725, version 2.0.2 used here
Adafruit_MCP4725 dac;


uint16_t val = 0;

void setup() {
    Serial.begin(9600);   // Start serial communication at 9600 baud rate
    dac.begin(0x60);
}

void loop() {
    if (Serial.available() > 0) {
        int number = Serial.parseInt();  // Read the received number
        if (number == 6666){ // random defined number to check Arduino connection in Chrolispp
          Serial.write(FIRMWARE_ID); 
          digitalWrite(LEDPIN, HIGH);
          delay(100);
          digitalWrite(LEDPIN, LOW);
          delay(100);
          digitalWrite(LEDPIN, HIGH);
          delay(100);
          digitalWrite(LEDPIN, LOW);
          delay(100);
          digitalWrite(LEDPIN, HIGH);
          delay(100);
          digitalWrite(LEDPIN, LOW);
        } else if(number >= 1 && number <= 4096) { // MCP4725 has 12-bit range
          val = (uint16_t)number;
          dac.setVoltage(val-1, false);
          Serial.write(val);
        }
    }
}

