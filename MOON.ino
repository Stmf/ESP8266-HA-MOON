#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define WLAN_SSID       "SSID"
#define WLAN_PASS       "password"

// WiFi upgrade
#define OTAUSER         "admin"
#define OTAPASSWORD     "password"
#define OTAPATH         "/firmware"
#define SERVERPORT      80

#define AIO_SERVER      "192.168.1.10"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "device"
#define AIO_KEY         "password"

int gpio_pin1 = 4;
int gpio_pin2 = 12;

String webPage = "";

ESP8266WebServer HttpServer(SERVERPORT);
ESP8266HTTPUpdateServer httpUpdater;

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Publish moonCurState = Adafruit_MQTT_Publish(&mqtt, "room/light/moon/state");
Adafruit_MQTT_Publish moonCurBrightness = Adafruit_MQTT_Publish(&mqtt, "room/light/moon/brightness");
Adafruit_MQTT_Publish moonCurColor = Adafruit_MQTT_Publish(&mqtt, "room/light/moon/color");

Adafruit_MQTT_Subscribe moonState = Adafruit_MQTT_Subscribe(&mqtt, "room/light/moon/state/set");
Adafruit_MQTT_Subscribe moonBrightness = Adafruit_MQTT_Subscribe(&mqtt, "room/light/moon/brightness/set");
Adafruit_MQTT_Subscribe moonColor = Adafruit_MQTT_Subscribe(&mqtt, "room/light/moon/color/set");

void MQTT_connect();

uint16_t moonStateValue = 0;
unsigned int powerState = 0;
uint16_t moonBrightValue = 0;
unsigned int brightnessState = 0;
uint16_t moonColorValue = 0;
unsigned int colorState = 0;

void setup() {

  Serial.begin(9600);
  
  pinMode(gpio_pin1, OUTPUT);
  digitalWrite(gpio_pin1, LOW);
  pinMode(gpio_pin2, OUTPUT);
  digitalWrite(gpio_pin2, LOW);
 
  httpUpdater.setup(&HttpServer, OTAPATH, OTAUSER, OTAPASSWORD);
  HttpServer.onNotFound(handleNotFound);
  HttpServer.begin();

  HttpServer.on("/", [](){
    webPage = "MOON";
    HttpServer.send(200, "text/html", webPage);
    webPage = "";
  });

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  // Setup MQTT subscription for ledBrightness feed.
  mqtt.subscribe(&moonState);
  mqtt.subscribe(&moonBrightness);
  mqtt.subscribe(&moonColor);
  
}

void loop() {

  HttpServer.handleClient();

  MQTT_connect();

  moonCurState.publish(powerState);
  
  int moonBrightValueOUT = map(brightnessState, 1, 1023, 0, 255);
  moonCurBrightness.publish(moonBrightValueOUT);
  
  moonCurColor.publish(colorState);

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(200))) {

    if (subscription == &moonColor) {
      moonColorValue = atoi((char *)moonColor.lastread);

      if(moonColorValue != colorState && powerState == 1) {
          if(moonColorValue == 0) {
            analogWrite(gpio_pin2, 0);
            analogWrite(gpio_pin1, brightnessState);
          }

          if(moonColorValue == 1) {
            analogWrite(gpio_pin1, 0);
            analogWrite(gpio_pin2, brightnessState);
          }
      }
      colorState = moonColorValue;
      
    }
    
    if (subscription == &moonState) {
      moonStateValue = atoi((char *)moonState.lastread);

      if(moonStateValue != powerState) {
        if(moonStateValue == 0) {
          analogWrite(gpio_pin2, 0);
          analogWrite(gpio_pin1, 0);
        }
        if(moonStateValue == 1) {

          brightnessState = 1023;
          
          if(colorState == 0) {
            analogWrite(gpio_pin2, 0);
            analogWrite(gpio_pin1, brightnessState);
          }

          if(colorState == 1) {
            analogWrite(gpio_pin1, 0);
            analogWrite(gpio_pin2, brightnessState);
          }

          moonCurState.publish(colorState);
        }
        powerState = moonStateValue;
      }
    }

    if (subscription == &moonBrightness) {
      moonBrightValue = atoi((char *)moonBrightness.lastread);
      moonBrightValue = map(moonBrightValue, 0, 255, 1, 1023);
      
      if(moonBrightValue != brightnessState) {
        
        brightnessState = moonBrightValue;

        if(powerState == 1) {
          if(colorState == 0) {
            analogWrite(gpio_pin2, 0);
            analogWrite(gpio_pin1, brightnessState);
          }
          
          if(colorState == 1) {
            analogWrite(gpio_pin1, 0);
            analogWrite(gpio_pin2, brightnessState);
          }
        }
        
        int moonBrightValueOUT = map(brightnessState, 1, 1023, 0, 255);
        moonCurBrightness.publish(moonBrightValueOUT);
      }
    }
    
  }
  
}

void handleNotFound() {
  HttpServer.send(404, "text/plain", "404: Not found");
}

void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
