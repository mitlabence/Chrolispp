#define OUTPIN 7
#define LEDPIN 13
#define FIRMWARE_ID 1  // 1 is digital, 2 is PWM, 3 is MCP4725 code for this project. Used to store in log file after recording

void setup() {
    Serial.begin(19200);   // Make sure baud rate agrees with Chrolis++ code
    pinMode(OUTPIN, OUTPUT);
}

void loop() {
    if (Serial.available() > 0) {
        int number = Serial.parseInt();  // Read the received number. If timeout, reads 0!
        if (number == 1) { // avoid time-out reset to LOW by assigning 1 = LOW
            digitalWrite(OUTPIN, LOW);
            digitalWrite(LEDPIN, LOW);
        } else if (number == 6666){ // random defined number to check Arduino connection in Chrolispp
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
        } else if(number > 0) { // anything apart from 1 (and time-out 0) (and 6666) should be "high"
          digitalWrite(OUTPIN, HIGH);
          digitalWrite(LEDPIN, HIGH);
        }
    }
}

