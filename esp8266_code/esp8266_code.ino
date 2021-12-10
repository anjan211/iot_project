#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/************************* Pin Definition *********************************/

//Relays for switching appliances
#define relay_1 D0
#define relay_2 D1

//Motor pins
#define in_1 D2
#define in_2 D3
#define en_1 D4

//Analog pin to read sensor value
#define sensor A0

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "anjanchatterjee"
#define AIO_KEY         "7ece82f8de734aad8bb7dbc92888747c"

/************ Global State (no need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiClientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup feed for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish photocell = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/photocell");
Adafruit_MQTT_Publish tempf = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/tempf");
Adafruit_MQTT_Publish tempc = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/tempc");

// Setup feed for subscribing to changes.
Adafruit_MQTT_Subscribe onoffbutton1 = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/relay1");
Adafruit_MQTT_Subscribe onoffbutton2 = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/relay2");
Adafruit_MQTT_Subscribe onoffbutton3 = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/dcmotorrot1");
Adafruit_MQTT_Subscribe onoffbutton4 = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/dcmotorrot2");

/*************************** Main Code ************************************/

void MQTT_connect();

void setup() {
  
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  
  Serial.begin(115200);
  delay(10);
  pinMode(relay_1,OUTPUT);
  pinMode(relay_2,OUTPUT);
  pinMode(in_1,OUTPUT);
  pinMode(in_2,OUTPUT);
  pinMode(en_1,OUTPUT);
  pinMode(sensor,INPUT);

  Serial.println(F("Adafruit MQTT project"));

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting ...... ");
  
  WiFiManager wm;
  
  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("iotAutomation","123456ac"); // password protected ap

  if(!res) {
      Serial.println("Failed to connect");
      // ESP.restart();
  } 
  else {
      //connected to the WiFi    
      Serial.println("connected...yeey :)");
  }
  
  // Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&onoffbutton1);
  mqtt.subscribe(&onoffbutton2);
  mqtt.subscribe(&onoffbutton3);
  mqtt.subscribe(&onoffbutton4);
}

uint32_t x=0;

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // this is our 'wait for incoming subscription packets' busy subloop

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &onoffbutton1) {
      Serial.print(F("Got Relay 1: "));
      Serial.println((char *)onoffbutton1.lastread);
      uint16_t state = atoi((char *)onoffbutton1.lastread);  // convert to a number
      digitalWrite(relay_1,state);
    }
      if (subscription == &onoffbutton2) {
      Serial.print(F("Got Relay 2: "));
      Serial.println((char *)onoffbutton2.lastread);
      uint16_t state = atoi((char *)onoffbutton2.lastread);  // convert to a number
      digitalWrite(relay_2,state);
    }
      if (subscription == &onoffbutton3) {
      Serial.print(F("Got DC Motor Rotation 1: "));
      Serial.println((char *)onoffbutton3.lastread);
      uint16_t state = atoi((char *)onoffbutton3.lastread);  // convert to a number
      analogWrite(en_1, state);   //sets the motors speed
      digitalWrite(in_1, HIGH);
      digitalWrite(in_2, LOW);
    }
      if (subscription == &onoffbutton4) {
      Serial.print(F("Got DC Motor Rotation 2: "));
      Serial.println((char *)onoffbutton4.lastread);
      uint16_t state = atoi((char *)onoffbutton4.lastread);  // convert to a number
      analogWrite(en_1, state);   //sets the motors speed
      digitalWrite(in_1, LOW);
      digitalWrite(in_2, HIGH);
    }
  }

//   Now we can publish stuff!
  Serial.print(F("\nSending photocell val "));
  Serial.print("Photocell"); 
  Serial.println(analogRead(sensor));
  Serial.print("...");
  int sensor_val = analogRead(sensor);

//  For LDR value
  if (! photocell.publish(sensor_val)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }

//  For temperature in Celsius
  float millivolts = (sensor_val/1024.0) * 3300; //3300 is the voltage provided by NodeMCU
  float celsius = millivolts/10;
  Serial.print("in DegreeC=   ");
  Serial.println(celsius);
  
  if (! tempc.publish(celsius)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }

//  For temperature in Fahrenheit
  float fahrenheit = ((celsius * 9)/5 + 32);
  Serial.print(" in Farenheit=   ");
  Serial.println(fahrenheit);

  if (! tempf.publish(fahrenheit)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }

  // delay(1000);

}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
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
         // basically die and wait for WDT to get reset
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
