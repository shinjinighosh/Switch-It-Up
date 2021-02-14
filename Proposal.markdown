<!-- ## Wiring

# TODOS
1. Design ESP32 to buttons wiring DONE
2. Design ESP32 to mic wiring DONE
3. FSM for buttons
4. FSM for mic + API
5. Motor - Lay
6. Writeup -->

# Switch-It-Up

**Shinjini Ghosh, Pawan Goyal, Lay Jain**


## Overview
We have a world of voice-enabled smart devices around us, assisting us in mundane and not-so-mundane daily tasks. However, smart lights still remain a hassle to use, often requiring rewiring of the switch, re-installation, taking apart existing systems, and so on 😕 .

Instead, in this project, we envision and develop a portable, low-cost, generalized smart switch which can be easily set up as an extension on any common existing switch system 😋. Think temporary hostels, semester-time dorm rooms, and overnight hotels - one Switch-It-Up device can unlock the wonders 😍 $\hspace{0.5mm}$ of voice-assisted light toggling within seconds.


## Switch-It-Up in Action
Simulation or Video

## Hardware

### Parts
1. ESP32 Board
2. TFT Display
3. Microphone
4. 2 buttons
5. Linear Motor / Actuator
6. Powerboard
7. Rectangular Wooden Frame (2" x 1")
8. Breadboard and Jumper Wires

### Wiring
The following is a schematic of how the different hardware components will be wired together in the final device.
![Wiring](https://i.imgur.com/Eg9Dhb8.jpg)



### Button Workings
We have two buttons on the device - primarily used as manual on/off buttons, but with software expansion, can be used for a combination of things (including brightness toggle if the switch allows for it), flickering lights, etc.

We define a button press as a full cycle of the button going from OPEN - CLOSED - OPEN, i.e., first pressing down and then releasing the button. However, just tracking a 0 to 1 state and vice-versa is not enough due to bouncing issues and hence, we use debouncing (for 10ms) on both ends. We also keep in a mechanism for differentiating short presses ($10$ ms - $1000$ ms) and long presses ($> 1000$ ms).

The FSM denoting these workings and the code used are as below.

![Button Press FSM](https://imgur.com/Gkzn1XM.png)

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


### Speech/Microphone
We use a microphone module to capture the voice commands of the user. The microphone module has $3.3$ V across its ends and provides a $12$ bit resolution, thus having $3.3/2^{12}$ voltage per increment. The measurements are taken at the frequency of $8$kHz. $8$KHz ensures right balance between quality and memory management. Though incresing frequency increses quality of sound, it significantly increases memory uptake and since our microcontroller has limited memory, (around $500$kB), memory management becomes crucial. We use google API to convert analog signals to text and the API requires minimum of $8$kHz frequency, thus our choice. 

However, even after these optimizations, our microcontroller could handle a voice input of length 5 seconds (assuming we could use around $60$kB out of $500$kB for voice processing). To improve that further, we make use of **MuLaw** that relies on the fact that human hearing tends to respond linearly to logarithmic changes in intensity. Mathematically the algorithm is implemented by taking an input value $x$ and converting it to an output value $z$ via:
$$y = \frac{x}{32768}$$
followed by
$$z = \text{sign}(y) \cdot \frac{\ln (1+\mu|y|)}{\ln (1+\mu)}$$

where $\mu = 255$ (for $8$-bit output) and $\text{sign}(y)$ is the sign function.
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

### Arm
The arm toggles the electric switch state `on/off` with a short stroke ($\approx 0.5$ inches) to the frame, which in turn is obtained from a linear voice coil actuator. The magnitude of this force (per unit length) $F=I \times B$, is proportional to the current $I$ (the permanent magnet flux density field   $B$ is fixed) in the and the direction of the displacement depends on the polarity of the input current. Thus, the electric switch can be toggled by changing the potential difference across the actuator.
![Actuator-Frame mechanism](https://i.imgur.com/KRwTqZA.jpg =500x)

We choose a moving coil DC linear mini-actuator ($\approx 5.0$ oz) which can deliver a force of $3.6$ N ($0.8$ lbs) at 100% duty, sufficient to flip an everyday electric switch (plus the wooden frame). Such a device is low power and small in dimensions ($\approx 1.2$" in diameter and $1.1$" in length), making our device lightweight and portable ✌️.




## Software
Code details

