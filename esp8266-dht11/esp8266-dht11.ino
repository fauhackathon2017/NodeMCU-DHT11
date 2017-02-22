 /**
 * IBM IoT Foundation managed Device
 * 
 * Author: Ant Elder
 * License: Apache License v2
 * 
 * updated by:  David Jaramillo
 * 
 */
#include <ESP8266WiFi.h>
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient/releases/tag/v2.3
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson/releases/tag/v5.0.7

// dht sensor library by Adafruit version 1.2.3 - newer ones don't compile
#include "DHT.h"
#define DHTPIN D2     // what digital pin we're connected to
#define DHTTYPE DHT11

//-------- Customise these values -----------

const char* ssid = "fau";
const char* password = "";

//https://<orgid>.internetofthings.ibmcloud.com/api/v0002/historian/types/<deviceType>/devices/<deviceId>

#define ORG "PUT-YOUR-ORG-HERE"
#define DEVICE_TYPE "YOUR-DEVICE-TYPE"
#define DEVICE_ID "YOUR-DEVICE-ID"
#define TOKEN "PUT-YOUR-TOKEN-HERE"
//-------- Customise the above values --------

char server[] = ORG ".messaging.internetofthings.ibmcloud.com";
char authMethod[] = "use-token-auth";
char token[] = TOKEN;
char clientId[] = "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID;

const char publishTopic[] = "iot-2/evt/status/fmt/json";
const char responseTopic[] = "iotdm-1/response";
const char manageTopic[] = "iotdevice-1/mgmt/manage";
const char updateTopic[] = "iotdm-1/device/update";
const char rebootTopic[] = "iotdm-1/mgmt/initiate/device/reboot";
const char ledStateTopic[]="iot-2/cmd/status/fmt/String";

void callback(char* topic, byte* payload, unsigned int payloadLength);
void wifiConnect();
void mqttConnect();
void initManagedDevice();
void publishData();
void handleUpdate();
void handleUpdate(byte* payload);
void handleLedState(byte* payload);

DHT dht(DHTPIN, DHTTYPE);
WiFiClient wifiClient;
PubSubClient client(server, 1883, callback, wifiClient);
void dhtRead( float *, float *, float *);

int publishInterval = 10000; // 10 seconds
long lastPublishMillis;

void blink( int );

void setup() {
 Serial.begin(9600); Serial.println();

 pinMode(BUILTIN_LED, OUTPUT); 
 pinMode(D3, OUTPUT); 
 pinMode(D4, OUTPUT); 

 wifiConnect();
 blink(1);
 mqttConnect();
 blink(2);
 initManagedDevice();
 blink(3); 
 Serial.println("DHT11 test!");
 Serial.print("Humidity: ");
 Serial.println("Temp: ");
 dht.begin();
}

void loop() {
 if (millis() - lastPublishMillis > publishInterval) {
 publishData(); 
 lastPublishMillis = millis();
 }

 if (!client.loop()) {
  mqttConnect();
  initManagedDevice();
 }
 
}

void dhtRead( float &h, float &t, float &f) {
    // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
   h = dht.readHumidity();
  // Read temperature as Celsius (the default)
   t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
   f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    h=53;
    t=23;
    f=76;
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  // float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  // float hic = dht.computeHeatIndex(t, h, false);

  //Serial.print(h); Serial.print("% ");
  //Serial.print(t); Serial.print("*C ");
  //Serial.print(f); Serial.println("*F ");
}

void wifiConnect() {
 Serial.print("Connecting to "); Serial.print(ssid);
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED) {
 delay(500);
 Serial.print(".");
 } 
 Serial.print("nWiFi connected, IP address: "); Serial.println(WiFi.localIP());
}

void mqttConnect() {
 if (!!!client.connected()) {
 Serial.print("Reconnecting MQTT client to "); Serial.println(server);
 while (!!!client.connect(clientId, authMethod, token)) {
 Serial.print(".");
 delay(500);
 }
 Serial.println();
 }
}

void initManagedDevice() {
 if (client.subscribe(responseTopic)) {
 Serial.println("subscribe to responses OK");
 } else {
 Serial.println("subscribe to responses FAILED");
 }

 if (client.subscribe(rebootTopic)) {
 Serial.println("subscribe to reboot OK");
 } else {
 Serial.println("subscribe to reboot FAILED");
 }

 if (client.subscribe(updateTopic)) {
 Serial.println("subscribe to update OK");
 } else {
 Serial.println("subscribe to update FAILED");
 }

 if (client.subscribe(ledStateTopic)) {
   Serial.println("subscribe to ledStateTopic OK");
 } else {
   Serial.println("subscribe to update FAILED");
 }
 

 StaticJsonBuffer<300> jsonBuffer;
 JsonObject& root = jsonBuffer.createObject();
 JsonObject& d = root.createNestedObject("d");
 JsonObject& metadata = d.createNestedObject("metadata");
 metadata["publishInterval"] = publishInterval;
 JsonObject& supports = d.createNestedObject("supports");
 supports["deviceActions"] = true;

 char buff[300];
 root.printTo(buff, sizeof(buff));
 Serial.println("publishing device metadata:"); Serial.println(buff);
 if (client.publish(manageTopic, buff)) {
 Serial.println("device Publish ok");
 } else {
 Serial.print("device Publish failed:");
 }
}

void publishData() {
 float h, t, f;
 dhtRead( h, t, f);
 String payload = "{\"d\":{\"counter\":";
 payload += millis() / 1000;
 payload +=",";
 payload +="\"Humid%\":";
 payload += h;
 payload +=",";
 //payload +="\"TempC\":";
 //payload += t;
 //payload +=",";
 payload +="\"TempF\":";
 payload += f;
 payload += "}}";

 Serial.print(h); Serial.print("% ");
 //Serial.print(t); Serial.print("*C ");
 Serial.print(f); Serial.println("*F ");
 
 Serial.print("Sending payload: "); Serial.println(payload);
 
 if (client.publish(publishTopic, (char*) payload.c_str())) {
 Serial.println("Publish OK");
 } else {
 Serial.println("Publish FAILED");
 }
 blink(1);
 payload="";
}

void callback(char* topic, byte* payload, unsigned int payloadLength) {
 
 Serial.print("callback invoked for topic: "); Serial.println(topic);

 if (strcmp (responseTopic, topic) == 0) {
 return; // just print of response for now 
 }

 if (strcmp (rebootTopic, topic) == 0) {
 Serial.println("Rebooting...");
 ESP.restart();
 }

 if (strcmp (updateTopic, topic) == 0) {
 handleUpdate(payload); 
 } 

 if (strcmp (ledStateTopic, topic) == 0) {
   char buff[200]="hello123123123";
   Serial.print("payload length: "); Serial.println(payloadLength);
   memset(buff,0,200);
   memcpy(buff,payload,payloadLength);
   Serial.println(buff);
   handleLedState(payload); 
 } 
 
}

void handleLedState(byte * payload) {
 blink(2);
 return;
 }


void handleUpdate(byte * payload) {
 StaticJsonBuffer<300> jsonBuffer;
 JsonObject& root = jsonBuffer.parseObject((char*)payload);
 if (!root.success()) {
 Serial.println("handleUpdate: payload parse FAILED");
 return;
 }

 blink(1);

 Serial.println("handleUpdate payload:"); root.prettyPrintTo(Serial); Serial.println();

 JsonObject& d = root["d"];
 JsonArray& fields = d["fields"];
 for(JsonArray::iterator it=fields.begin(); it!=fields.end(); ++it) {
 JsonObject& field = *it;
 const char* fieldName = field["field"];
 if (strcmp (fieldName, "metadata") == 0) {
 JsonObject& fieldValue = field["value"];
 if (fieldValue.containsKey("publishInterval")) {
 publishInterval = fieldValue["publishInterval"];
 Serial.print("publishInterval:"); Serial.println(publishInterval);
 }
 }
 }
}

void blink ( int num ) {
  Serial.print("Blink: ");  Serial.println(num);
  for (int count = 1; count<= num; count++) { 
     digitalWrite(BUILTIN_LED, LOW);
     delay(500);
     digitalWrite(BUILTIN_LED, HIGH);
  }
}
