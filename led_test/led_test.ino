
const int GREEN_LIGHT = 16;
const int RED_LIGHT = 17;


// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(GREEN_LIGHT, OUTPUT);
  pinMode(RED_LIGHT, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(GREEN_LIGHT, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(GREEN_LIGHT, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);                       // wait for a second
  digitalWrite(RED_LIGHT, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(RED_LIGHT, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);                       // wait for a second
}
