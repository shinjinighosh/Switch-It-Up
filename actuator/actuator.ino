//#include <SPI.h>
//#include <TFT_eSPI.h>
//TFT_eSPI tft = TFT_eSPI();
//
//const int BUTTON_PIN_1 = 16;
//const int BUTTON_PIN_2 = 17;
//const int INA = 26;
//const int INB = 27;
//const int MOTOR_ENABLE = 14;
//uint8_t bulb_state = 0;
//class Button {
//  public:
//    uint32_t t_of_state_2;
//    uint32_t t_of_button_change;
//    uint32_t debounce_time;
//    uint32_t long_press_time;
//    uint8_t pin;
//    uint8_t flag;
//    bool button_pressed;
//    uint8_t state; // This is public for the sake of convenience
//    Button(int p) {
//      flag = 0;
//      state = 0;
//      pin = p;
//      t_of_state_2 = millis(); //init
//      t_of_button_change = millis(); //init
//      debounce_time = 10;
//      long_press_time = 1000;
//      button_pressed = 0;
//    }
//    void read() {
//      uint8_t button_state = digitalRead(pin);
//      button_pressed = !button_state;
//    }
//
//
//    int update() {
//      read();
//      flag = 0;
//      if (state == 0) { // Unpressed, rest state
//        if (button_pressed) {
//          state = 1;
//          t_of_button_change = millis();
//        }
//      } else if (state == 1) { //Tentative pressed
//        if (!button_pressed) {
//          state = 0;
//          t_of_button_change = millis();
//        } else if (millis() - t_of_button_change >= debounce_time) {
//          state = 2;
//          t_of_state_2 = millis();
//        }
//      } else if (state == 2) { // Short press
//        if (!button_pressed) {
//          state = 4;
//          t_of_button_change = millis();
//        } else if (millis() - t_of_state_2 >= long_press_time) {
//          state = 3;
//        }
//      } else if (state == 3) { //Long press
//        if (!button_pressed) {
//          state = 4;
//          t_of_button_change = millis();
//        }
//      } else if (state == 4) { //Tentative unpressed
//        if (button_pressed && millis() - t_of_state_2 < long_press_time) {
//          state = 2; // Unpress was temporary, return to short press
//          t_of_button_change = millis();
//        } else if (button_pressed && millis() - t_of_state_2 >= long_press_time) {
//          state = 3; // Unpress was temporary, return to long press
//          t_of_button_change = millis();
//        } else if (millis() - t_of_button_change >= debounce_time) { // A full button push is complete
//          state = 0;
//          if (millis() - t_of_state_2 < long_press_time) { // It is a short press
//            flag = 1;
//          } else {  // It is a long press
//            flag = 2;
//          }
//        }
//      }
//      return flag;
//    }
//};
//
//Button button_1(BUTTON_PIN_1);
//Button button_2(BUTTON_PIN_2);
//
//void setup() {
//  Serial.begin(115200);               // Set up serial port
//  pinMode(BUTTON_PIN_1, INPUT_PULLUP);
//  pinMode(BUTTON_PIN_2, INPUT_PULLUP);
//  pinMode(INA, OUTPUT);
//  pinMode(INB, OUTPUT);
//  tft.init();
//  tft.setRotation(2);
//  tft.setTextSize(1);
//  tft.fillScreen(TFT_BLACK);
//  tft.setCursor(40, 60, 4);
//  tft.print("OFF");
//  digitalWrite(INA, LOW);
//  digitalWrite(INB, HIGH);
//  digitalWrite(MOTOR_ENABLE, LOW); // disable the motor
//}
//
//void loop() {
//  uint8_t flag_1 = button_1.update();
//  uint8_t flag_2 = button_2.update();
//  if (bulb_state == 0 && (flag_2 == 1 || flag_2 == 2)){
//    tft.fillScreen(TFT_BLACK);
//    tft.setCursor(40, 60, 4);
//    tft.print("ON");
//    digitalWrite(INA, HIGH);
//    digitalWrite(INB, LOW);
//    digitalWrite(MOTOR_ENABLE, HIGH);
//    delay(2000);
//    bulb_state = 1;
//  }
//  else if (bulb_state == 1 && (flag_1 == 1 || flag_1 == 2)){
//    tft.fillScreen(TFT_BLACK);
//    tft.setCursor(40, 60, 4);
//    tft.print("OFF");
//    digitalWrite(INA, LOW);
//    digitalWrite(INB, HIGH);
//    digitalWrite(MOTOR_ENABLE, HIGH);
//    delay(2000);
//    bulb_state = 0;
//  }
//}
