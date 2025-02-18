#define OUTPIN 7
#define LEDPIN 13
#include <Wire.h>
#include <Adafruit_MCP4725.h>

Adafruit_MCP4725 dac;


uint16_t val = 0;

void setup() {
    Serial.begin(9600);   // Start serial communication at 9600 baud rate
    pinMode(OUTPIN, OUTPUT);
    dac.begin(0x60);
}

void loop() {
    if (Serial.available() > 0) {
        int number = Serial.parseInt();  // Read the received number
        if (number == 0) {
            digitalWrite(OUTPIN, LOW);
            digitalWrite(LEDPIN, LOW);
        } else if (number == 6666){ // random defined number to check Arduino connection in Chrolispp
          Serial.write(1);
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
        }
    }
}

