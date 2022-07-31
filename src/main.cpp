#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <keys.h>
#include <FastLED.h>
#include <soc/soc.h>
#include <soc/rtc_cntl_reg.h>

#define LED_PIN 13 
#define NUM_LED 74
CRGBArray<NUM_LED> leds;

#define SOLID 0
#define PCMR 1
#define PRIDE 2
#define COLOR_FADE 3
#define FX_TOTAL 4
int fx = 0;
bool isXmasSet = false;
int fadeHue = 100;

int pcmrHue = 0;
int bgt = 200;
String rgbStr1 = "0000FF";

WiFiClient wiFiClient;
PubSubClient pubSubClient(wiFiClient);
long lastMessage = 0;
char msg[100];
int value = 0;

const char* LB = "-------------------------------";

int roller(int item) {
  if (item > 255) {
    item = 0;
  } else if (item < 0) {
    item = 255;
  }
  return item;
}

char* toChars(String strToConver) {
  int len = strToConver.length() + 1;
  char strArray[len];
  strToConver.toCharArray(strArray, len);
  return strArray;
}

unsigned long toColorCode(String colorHex) {
  int len = colorHex.length() + 1;
  char hexArray[len];
  colorHex.toCharArray(hexArray, len);
  unsigned long color = strtoul(hexArray, NULL, 16);
  return color;
}

void setFx(String selectedFx) {
  bool isValidFx = true;
  if (selectedFx == "solid") {
    fx = SOLID;
  } else if(selectedFx == "pcmr") {
    fx = PCMR;
  } else if(selectedFx == "pride") {
    fx = PRIDE;
  } else if(selectedFx == "color_fade") {
    fx = COLOR_FADE;
  } else {
    pubSubClient.publish("error/huebol/jar", "Invalid Effect", 2);
    isValidFx = false;
  }

  if (isValidFx) {
    char* fxArray = toChars(selectedFx);
    pubSubClient.publish("huebol/jar/fx/state", fxArray, 1);
  }
  
}

void displayLights() {
  FastLED.setBrightness(bgt);
  switch (fx) {

    case PCMR:
        fill_rainbow(leds, NUM_LED, pcmrHue, 2);
        pcmrHue++;
        if (pcmrHue > 255) {
          pcmrHue = 0;
        }
        
      break;

    case PRIDE:
      fill_rainbow(leds, NUM_LED, pcmrHue, 2);
      break;

    case COLOR_FADE:
      for(int i = 0; i < NUM_LED; i++) {
        leds[i] = CRGB().setHSV(fadeHue, 250, 250);
      }
      break;
    
    default:
      for(int i = 0; i < NUM_LED; i++) {
        leds[i] = CRGB().setColorCode(toColorCode(rgbStr1));
      }
      break;
  }

  FastLED.show();
}


// _________________________________________________________________

void callback(char* topic, byte* message, unsigned int lenght) {
  String messageTemp;
  for (size_t i = 0; i < lenght; i++) {
    messageTemp += (char) message[i];
  }

  String topicStr(topic);
  String category = topicStr.substring(11); // 16
  Serial.println(category);
  char* messageArray = toChars(messageTemp);

  if (category == "fx") {
    setFx(messageTemp);
  } else if (category == "rgb1") {
    rgbStr1 = messageTemp;
    pubSubClient.publish("huebol/jar/rgb1/state", messageArray, 1);
  } else if (category == "bgt") {
    bgt = messageTemp.toInt();
    pubSubClient.publish("huebol/jar/bgt/state", messageArray, 1);
  }
  
  delay(100);
  Serial.print("Topic in: ");
  Serial.println(topic);
  Serial.print("Message in: ");
  Serial.println(messageTemp);
  // delay(200);
}

void setupWifi() {
  delay(12);
  Serial.println(LB);
  Serial.print("Connecting to: ");
  Serial.println(SSID);

  WiFi.begin(SSID, PWD);

  while (WiFi.status() != WL_CONNECTED){
    delay(420);
    Serial.print(">");
  }

  Serial.println();
  Serial.println("Connected to WiFi!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void setupPubSub() {
  pubSubClient.setServer(MQTT_SERVER, 1883);
  pubSubClient.setCallback(callback);
}

void reconnectToPubSub() {
  while(!pubSubClient.connected()) {
    Serial.println("MQTT Connecting...");
    if (pubSubClient.connect("HuebolJar")) {
      Serial.println("Connected to MQTT!");
      pubSubClient.subscribe("huebol/jar/#");
    } else {
      Serial.println("Failed to connect to MQTT");
      Serial.print("State: ");
      Serial.println(pubSubClient.state());
      Serial.println("Trying again in 5 seconds...");
      delay(5000);
    }
  }
}

void setupLeds() {
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LED);
  // FastLED.setMaxPowerInVoltsAndMilliamps(5, 1000);
  FastLED.setBrightness(bgt);
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);

  setupWifi();
  setupPubSub();
  setupLeds();

  delay(1500);

  pubSubClient.publish("huebol/jar/boot", "1", 2);
}

void loop() {

  if (!pubSubClient.connected()){
    reconnectToPubSub();
  }
  pubSubClient.loop();

  EVERY_N_MILLISECONDS(20) { displayLights(); }
  EVERY_N_MINUTES(10) { fadeHue = roller(fadeHue + 1); }
  
}