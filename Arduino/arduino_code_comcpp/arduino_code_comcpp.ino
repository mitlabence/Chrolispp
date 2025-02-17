#define OUTPIN 7
#define LEDPIN 13

void setup() {
    Serial.begin(9600);   // Start serial communication at 9600 baud rate
    pinMode(OUTPIN, OUTPUT);
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
        } else { // anything apart from 0 (and 6666) should be "high"
          digitalWrite(OUTPIN, HIGH);
          digitalWrite(LEDPIN, HIGH);
        }
    }
}

