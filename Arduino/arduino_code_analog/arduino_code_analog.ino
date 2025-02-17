#define COMTEST_VALUE 6666  // the value to prompt arduino COM feedback (for testing COM connection)
#define LEDPIN 13
int analogPin = 6; 
int receivedVal = 0; // 1 - 256 to set levels 0-255

void setup() {
  Serial.begin(9600);   // Start serial communication at 9600 baud rate
  pinMode(analogPin, OUTPUT); // set PWM pin (pin 6: 980 Hz) as output. Range 0-255 = 0-5V?
}

void loop() {
    if (Serial.available() > 0) {
      receivedVal = Serial.parseInt();  // Read the received number. On timeout, this reads 0.
      if (receivedVal == COMTEST_VALUE){ // random defined number to check Arduino connection in Chrolispp
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
      } 
      else if (receivedVal >= 1 && receivedVal <= 256) { // shift by +1 to ignore timeout 0 value
        analogWrite(analogPin, receivedVal-1);
      }
    
  }
}