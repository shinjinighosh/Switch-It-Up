#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFiClientSecure.h>
//WiFiClientSecure is a big library. It can take a bit of time to do that first compile

TFT_eSPI tft = TFT_eSPI();

const int DELAY = 1000;
const int SAMPLE_FREQ = 8000;                          // Hz, telephone sample rate
const float SAMPLE_DURATION = 2;// 5.5;                        // duration of fixed sampling (seconds)
const int NUM_SAMPLES = SAMPLE_FREQ * SAMPLE_DURATION;  // number of of samples
const int ENC_LEN = (NUM_SAMPLES + 2 - ((NUM_SAMPLES + 2) % 3)) / 3 * 4;  // Encoded length of clip

const uint16_t RESPONSE_TIMEOUT = 6000;
const uint16_t OUT_BUFFER_SIZE = 1000; //size of buffer to hold HTTP response
char response[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request

/* CONSTANTS */
//Prefix to POST request:
const char PREFIX[] = "{\"config\":{\"encoding\":\"MULAW\",\"sampleRateHertz\":8000,\"languageCode\": \"en-US\", \"speechContexts\":[{\"phrases\":[\"switch\", \"on\", \"off\"]}]}, \"audio\": {\"content\":\"";
const char SUFFIX[] = "\"}}"; //suffix to POST request
const int AUDIO_IN = A0; //pin where microphone is connected
const char API_KEY[] = "AIzaSyAXCndiHqZS3syF4OHO6VzDRJ6o3L3rIwM";

const uint8_t PIN_1 = 19; //button 1
const uint8_t PIN_2 = 5; //button 2
const int INA = 26;
const int INB = 27;
const int MOTOR_ENABLE = 14;

/* Global variables*/
uint8_t button_state; //used for containing button state and detecting edges
int old_button_state; //used for detecting button edges
uint32_t time_since_sample;      // used for microsecond timing


char speech_data[ENC_LEN + 200] = {0}; //global used for collecting speech data
const char* NETWORK     =  "MIT"; // "EECS_Labs";     // your network SSID (name of wifi network)
const char* PASSWORD = ""; // your network password
const char*  SERVER = "speech.google.com";  // Server URL

uint8_t old_val;
uint32_t timer;

WiFiClientSecure client; //global WiFiClient Secure object

void setup() {
  Serial.begin(115200);               // Set up serial port
  //  digitalWrite(14, 1); // to keep LED backlight on at setup

  tft.init();  //init screen
  tft.setRotation(2); //adjust rotation
  tft.setTextSize(1); //default font size
  tft.fillScreen(TFT_BLACK); //fill background
  tft.setTextColor(TFT_GREEN, TFT_BLACK); //set color of font to green foreground, black background
  Serial.begin(115200); //begin serial comms
  delay(100); //wait a bit (100 ms)
  pinMode(PIN_1, INPUT_PULLUP);
  pinMode(PIN_2, INPUT_PULLUP);
  pinMode(14, OUTPUT);
  pinMode(INA, OUTPUT);
  pinMode(INB, OUTPUT);
  pinMode(MOTOR_ENABLE, OUTPUT);

  WiFi.begin(NETWORK, PASSWORD); //attempt to connect to wifi
  uint8_t count = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(NETWORK);
  while (WiFi.status() != WL_CONNECTED && count < 12) {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.printf("%d:%d:%d:%d (%s) (%s)\n", WiFi.localIP()[3], WiFi.localIP()[2],
                  WiFi.localIP()[1], WiFi.localIP()[0],
                  WiFi.macAddress().c_str() , WiFi.SSID().c_str());
    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }
  timer = millis();
  old_val = digitalRead(PIN_1);
}

//main body of code
void loop() {
  Serial.println("Loop Started");
  button_state = digitalRead(PIN_1);
  // Serial.println(button_state); UP: 1, DOWN: 0
  // if (!button_state && button_state != old_button_state) {
  if (1){ // @dev: Lay
    Serial.println("listening...");
    record_audio();
    Serial.println("sending...");
    Serial.print("\nStarting connection to server...");
    //delay(300);
    bool conn = false;
    for (int i = 0; i < 10; i++) {
      int val = (int)client.connect(SERVER, 443);
      Serial.print(i); Serial.print(": "); Serial.println(val);
      if (val != 0) {
        conn = true;
        break;
      }
      Serial.print(".");
      //delay(300);
    }
    if (!conn) {
      Serial.println("Connection failed!");
      return;
    } else {
      Serial.println("Connected to server!");
      Serial.println(client.connected());
      int len = strlen(speech_data);
      // Make a HTTP request:
      client.print("POST /v1/speech:recognize?key="); client.print(API_KEY); client.print(" HTTP/1.1\r\n");
      client.print("Host: speech.googleapis.com\r\n");
      client.print("Content-Type: application/json\r\n");
      client.print("cache-control: no-cache\r\n");
      client.print("Content-Length: "); client.print(len);
      client.print("\r\n\r\n");
      int ind = 0;
      int jump_size = 1000;
      char temp_holder[jump_size + 10] = {0};
      Serial.println("sending data");
      while (ind < len) {
        delay(80);//experiment with this number!
        //if (ind + jump_size < len) client.print(speech_data.substring(ind, ind + jump_size));
        strncat(temp_holder, speech_data + ind, jump_size);
        client.print(temp_holder);
        ind += jump_size;
        memset(temp_holder, 0, sizeof(temp_holder));
      }
      client.print("\r\n");
      //Serial.print("\r\n\r\n");
      Serial.println("Through send...");
      unsigned long count = millis();
      while (client.connected()) {
        Serial.println("IN!");
        String line = client.readStringUntil('\n');
        Serial.print(line);
        if (line == "\r") { //got header of response
          Serial.println("headers received");
          break;
        }
        if (millis() - count > RESPONSE_TIMEOUT) break;
      }
      Serial.println("");
      Serial.println("Response...");
      count = millis();
      while (!client.available()) {
        delay(100);
        Serial.print(".");
        if (millis() - count > RESPONSE_TIMEOUT) break;
      }
      Serial.println();
      Serial.println("-----------");
      memset(response, 0, sizeof(response));
      while (client.available()) {
        char_append(response, client.read(), OUT_BUFFER_SIZE);
      }
      Serial.println(response);
      char* trans_id = strstr(response, "transcript");
      if (trans_id != NULL) {
        char* foll_coll = strstr(trans_id, ":");
        char* starto = foll_coll + 2; //starting index
        char* endo = strstr(starto + 1, "\""); //ending index
        int transcript_len = endo - starto + 1;
        char transcript[100] = {0};
        strncat(transcript, starto, transcript_len);
        Serial.println(transcript);
        commands(transcript);
      }
      Serial.println("-----------");
      client.stop();
      Serial.println("done");
    }
  }
  old_button_state = button_state;
}

//function used to record audio at sample rate for a fixed nmber of samples
void record_audio() {
  int sample_num = 0;    // counter for samples
  int enc_index = strlen(PREFIX) - 1;  // index counter for encoded samples
  float time_between_samples = 1000000 / SAMPLE_FREQ;
  int value = 0;
  char raw_samples[3];   // 8-bit raw sample data array
  memset(speech_data, 0, sizeof(speech_data));
  sprintf(speech_data, "%s", PREFIX);
  char holder[5] = {0};
  Serial.println("starting");
  uint32_t text_index = enc_index;
  uint32_t start = millis();
  time_since_sample = micros();
  //while (sample_num < NUM_SAMPLES && !digitalRead(PIN_1)) { //read in NUM_SAMPLES worth of audio data
  while(sample_num < NUM_SAMPLES) { // @dev: Lay
    value = analogRead(AUDIO_IN);  //make measurement
    raw_samples[sample_num % 3] = mulaw_encode(value - 1241); //remove 1.0V offset (from 12 bit reading)
    sample_num++;
    if (sample_num % 3 == 0) {
      base64_encode(holder, raw_samples, 3);
      strncat(speech_data + text_index, holder, 4);
      text_index += 4;
    }
    // wait till next time to read
    while (micros() - time_since_sample <= time_between_samples); //wait...
    time_since_sample = micros();
  }
  Serial.println(millis() - start);
  sprintf(speech_data + strlen(speech_data), "%s", SUFFIX);
  Serial.println("out");
}


int8_t mulaw_encode(int16_t sample) {
  //paste the fast one here.
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
    digitalWrite(MOTOR_ENABLE, HIGH);
    delay(2000);
    digitalWrite(MOTOR_ENABLE, LOW);
    }
    bool turnOn = false;
    if (strstr(tr, "on") != NULL || strstr(tr, "On") != NULL) turnOn = true;
    if (turnOn) {
    tft.fillScreen(TFT_BLACK);
    tft.println("Light is On");
    digitalWrite(INA, HIGH);
    digitalWrite(INB, LOW);
    digitalWrite(MOTOR_ENABLE, HIGH);
    delay(2000);
    digitalWrite(MOTOR_ENABLE, LOW);
    }
  }
}
