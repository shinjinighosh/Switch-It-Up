# Switch-It-Up

**Shinjini Ghosh, Pawan Goyal, Lay Jain**

## Overview

We have a world of voice-enabled smart devices around us, assisting us in mundane and not-so-mundane daily tasks. However, smart lights still remain a hassle to use, often requiring rewiring of the switch, re-installation, taking apart existing systems, and so on 😕 .

Instead, in this project, we envision and develop a portable, low-cost, generalized smart switch which can be easily set up as an extension on any common existing switch system 😋. Think temporary hostels, semester-time dorm rooms, and overnight hotels - one Switch-It-Up device can unlock the wonders 😍 $\\hspace{0.5mm}$ of voice-assisted light toggling within seconds.

## Switch-It-Up in Action

Simulation or Video

## Hardware

### Parts

1.  ESP32 Board
2.  TFT Display
3.  Microphone
4.  2 LEDs
5.  Servo Motor & Chip
6.  Power Supply
7.  Rectangular Wooden Frame (2" x 1")
8.  Breadboard and Jumper Wires

### Wiring

The following is a schematic of how the different hardware components will be wired together in the final device.
![Wiring](https://i.imgur.com/Eg9Dhb8.jpg)

### Button Workings

We have two buttons on the device - primarily used as manual on/off buttons, but with software expansion, can be used for a combination of things (including brightness toggle if the switch allows for it), flickering lights, etc.

We define a button press as a full cycle of the button going from OPEN - CLOSED - OPEN, i.e., first pressing down and then releasing the button. However, just tracking a 0 to 1 state and vice-versa is not enough due to bouncing issues and hence, we use debouncing (for 10ms) on both ends. We also keep in a mechanism for differentiating short presses ($10$ ms - $1000$ ms) and long presses ($> 1000$ ms).

Here is an FSM denoting this working.

![Button Press FSM](https://imgur.com/Gkzn1XM.png)

The code below implements this FSM.

```c=
#include <SPI.h>
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();

const int BUTTON_PIN = 16;

class Button {
  public:
    uint32_t t_of_state_2;
    uint32_t t_of_button_change;
    uint32_t debounce_time;
    uint32_t long_press_time;
    uint8_t pin;
    uint8_t flag;
    bool button_pressed;
    uint8_t state;

    Button(int p) {
      flag = 0;
      state = 0;
      pin = p;
      t_of_state_2 = millis(); //init
      t_of_button_change = millis(); //init
      debounce_time = 10;
      long_press_time = 1000;
      button_pressed = 0;
    }

    void read() {
      uint8_t button_state = digitalRead(pin);
      button_pressed = !button_state;
    }


    int update() {
      read();
      flag = 0;
      if (state == 0) { // Unpressed, rest state
        if (button_pressed) {
          state = 1;
          t_of_button_change = millis();
        }
      } else if (state == 1) { //Tentative pressed
        if (!button_pressed) {
          state = 0;
          t_of_button_change = millis();
        } else if (millis() - t_of_button_change >= debounce_time) {
          state = 2;
          t_of_state_2 = millis();
        }
      } else if (state == 2) { // Short press
        if (!button_pressed) {
          state = 4;
          t_of_button_change = millis();
        } else if (millis() - t_of_state_2 >= long_press_time) {
          state = 3;
        }
      } else if (state == 3) { //Long press
        if (!button_pressed) {
          state = 4;
          t_of_button_change = millis();
        }
      } else if (state == 4) { //Tentative unpressed
        if (button_pressed && millis() - t_of_state_2 < long_press_time) {
          state = 2; // Unpress was temporary, return to short press
          t_of_button_change = millis();
        } else if (button_pressed && millis() - t_of_state_2 >= long_press_time) {
          state = 3; // Unpress was temporary, return to long press
          t_of_button_change = millis();
        } else if (millis() - t_of_button_change >= debounce_time) { // A full button push is complete
          state = 0;
          if (millis() - t_of_state_2 < long_press_time) { // It is a short press
            flag = 1;
          } else {  // It is a long press
            flag = 2;
          }
        }
      }
      return flag;
    }
};
```

Now, we can identify short presses and long presses. Here, button A corresponds to the ON button and button B corresponds to the OFF button. Pressing button A currently prints "Light turns ON" on the TFT display and pressing button B currently prints "Light turns OFF" on the TFT display. Once we have the motor parts, we can simply link the button presses to moving the motor in addition to printing on the TFT display. In the future, when we wish to extend the system to various brightness levels or mode toggling, we can utilize the long press option, which we have already developed and tested.

The code snippet below uses instances of the `Button` class to implement our override switches. The current state of the light switch is displayed on the TFT display for want of an actuator in our possession currently.

```c=
Button button_1(BUTTON_PIN_1);
Button button_2(BUTTON_PIN_2);

void setup() {
  Serial.begin(115200);               // Set up serial port
  pinMode(BUTTON_PIN_1, INPUT_PULLUP);
  pinMode(BUTTON_PIN_2, INPUT_PULLUP);
  tft.init();
  tft.setRotation(2);
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(40, 60, 4);
  tft.print("OFF");

}

void loop() {
  uint8_t flag_1 = button_1.update();
  uint8_t flag_2 = button_2.update();
  if (bulb_state == 0 && (flag_2 == 1 || flag_2 == 2)){
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(40, 60, 4);
    tft.print("ON");
    bulb_state = 1;
  }
  else if (bulb_state == 1 && (flag_1 == 1 || flag_1 == 2)){
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(40, 60, 4);
    tft.print("OFF");
    bulb_state = 0;
  }
}
```

### Speech/Microphone

We use a microphone module to capture the voice commands of the user. The microphone module has $3.3$ V across its ends and provides a $12$ bit resolution, thus having $3.3/2^{12}$ voltage per increment. The measurements are taken at the frequency of $8$kHz. $8$KHz ensures right balance between quality and memory management. Though incresing frequency increses quality of sound, it significantly increases memory uptake and since our microcontroller has limited memory, (around $500$kB), memory management becomes crucial. We use google API to convert analog signals to text and the API requires minimum of $8$kHz frequency, thus our choice.

However, even after these optimizations, our microcontroller could handle a voice input of length 5 seconds (assuming we could use around $60$kB out of $500$kB for voice processing). To improve that further, we make use of **MuLaw** that relies on the fact that human hearing tends to respond linearly to logarithmic changes in intensity. Mathematically the algorithm is implemented by taking an input value $x$ and converting it to an output value $z$ via:
$$y = \\frac{x}{32768}$$
followed by
$$z = \\text{sign}(y) \\cdot \\frac{\\ln (1+\\mu|y|)}{\\ln (1+\\mu)}$$

where $\\mu = 255$ (for $8$-bit output) and $\\text{sign}(y)$ is the sign function.
The first equation converts the input between $-1$ and $1$ and then second equation converts it between $-128$ and $127$ ($8$ bit), thus downscaling a $16$ bit input ($12$ bit input is converted to $16$bit for Google API to work) to $8$ bits. This provides our micro-controller with the capacity to process voice commands as long as 10 seconds before it sends a GET request.
Finally, we use Google speech API to find the transcript associated with the captured analog signal. The Google speech API returns a JSON of the format

```json=
{
    "results": [
        {
            "alternatives": [
                {
                    "transcript": "What has gone wrong?",
                    "confidence": 0.71339726 }
            ]
        }
    ]
}
```

```c=
int8_t mulaw_encode(int16_t sample) {
  const uint16_t MULAW_MAX = 0x1FFF;
  const uint16_t MULAW_BIAS = 33;
  uint16_t mask = 0x1000;
  uint8_t sign = 0;
  uint8_t position = 12;
  uint8_t lsb = 0;
  if (sample < 0)
  {
    sample = -sample;
    sign = 0x80;
  }
  sample += MULAW_BIAS;
  if (sample > MULAW_MAX)
  {
    sample = MULAW_MAX;
  }
  for (; ((sample & mask) != mask && position >= 5); mask >>= 1, position--)
    ;
  lsb = (sample >> (position - 4)) & 0x0f;
  return (~(sign | ((position - 5) << 4) | lsb));
}
```

### Arm

The arm toggles the electric switch state `on/off` with a short stroke ($\\approx 0.5$ inches) to the frame, which in turn is obtained from a linear voice coil actuator. The magnitude of this force (per unit length) $F=I \\times B$, is proportional to the current $I$ (the permanent magnet flux density field   $B$ is fixed) in the and the direction of the displacement depends on the polarity of the input current. Thus, the electric switch can be toggled by changing the potential difference across the actuator.
![Actuator-Frame mechanism](<https://i.imgur.com/KRwTqZA.jpg =500x>)

We choose a moving coil DC linear mini-actuator ($\\approx 5.0$ oz) which can deliver a force of $3.6$ N ($0.8$ lbs) at 100% duty, sufficient to flip an everyday electric switch (plus the wooden frame). Such a device is low power and small in dimensions ($\\approx 1.2$" in diameter and $1.1$" in length), making our device lightweight and portable ✌️.

Switch-It-Up parses the audio user input and flips the relavant switches accordingly. The code below implements this functionality. We display the Switch states on the TFT screen for we do not have an actuator in our inventory right now, but below we also have the code in place for the actuator to work out-of-the-box once we obtain one and wire it up according to the schematic.

```c=
void commands(char* tr) {
  bool isSwitchCommand = false;
  if (strstr(tr, "Switch") != NULL || strstr(tr, "switch") != NULL) isSwitchCommand = true;

  if (isSwitchCommand) {
    bool turnOff = false;
    if (strstr(tr, "off") != NULL || strstr(tr, "Off") != NULL) turnOff = true;
    if (turnOff) {
    tft.fillScreen(TFT_BLACK);
      tft.println("Light is Off");
      digitalWrite(INA, LOW);
      digitalWrite(INB, HIGH);
    }
    bool turnOn = false;
    if (strstr(tr, "on") != NULL || strstr(tr, "On") != NULL) turnOn = true;
    if (turnOn) {
    tft.fillScreen(TFT_BLACK);
      tft.println("Light is On");
      digitalWrite(INA, HIGH);
      digitalWrite(INB, LOW);
    }
  }
}
```

The full code can be found on our GitHub repository [here](https://github.com/shinjinighosh/Switch-It-Up) 😊.
