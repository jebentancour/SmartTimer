
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <Arduino.h>
#include <Ticker.h>
#include <EEPROM.h>
#include <PubSubClient.h>

#include "FSWebServer.h"
#include "Config.h"
#include "DynamicData.h"
#include "power.h"

#define BUTTON          0
#define RELAY           12
#define LED             15

Ticker flipper;
void flip() { digitalWrite(LED, !digitalRead(LED)); }

char message_buff[60];
char topic_buff[60];
char client_buff[60];

int kPwrUpdFreq = 20;

unsigned long TTasks;
unsigned long longPress;

ESP8266PowerClass power_read;

WiFiClient wifiClient;
WiFiClientSecure wifiClientSecure;
WiFiClientSecure oemClient;

PubSubClient mqttClient;

// Callback function
boolean pub = true;
void callback(char* topic, byte* payload, unsigned int len) {  
  Serial.print("Recieved: ");
  Serial.write(payload, len);
  Serial.println();  
  char* data = (char*)payload;  
  if (strncmp(data,"OFF",3) == 0){
    setRelay(LOW);
  }
  if (strncmp(data,"ON",2) == 0){
    setRelay(HIGH);
  }
}

void toggle() {
  detachInterrupt(digitalPinToInterrupt(BUTTON));
  int i = 0;
  while((digitalRead(BUTTON) == LOW)&&(i < 10)){
    delayMicroseconds(100);
    i++;
  }  
  if (i >= 10) {
    if (digitalRead(RELAY)) {
      setRelay(LOW);
    }else{
      setRelay(HIGH);
    }
  }
  attachInterrupt(BUTTON,toggle,FALLING);
}

void setRelay(boolean state){
  if (state) {
    EEPROM.write(0x00, 0x01);
  } else {
    EEPROM.write(0x00, 0x00);
  }
  EEPROM.commit();
  digitalWrite(RELAY, state);
  pub = true;
}

boolean reset = false;
boolean firstConnect = true;
int wifiMode;
int k;

void setup(void){  
  pinMode(RELAY, OUTPUT);
  EEPROM.begin(0x08);
  int lastRelayState = EEPROM.read(0x00);
  if (lastRelayState == 0x01) {
     digitalWrite(RELAY, HIGH);
  } else {
     digitalWrite(RELAY, LOW);
  }

  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  pinMode(BUTTON, INPUT);
  attachInterrupt(BUTTON,toggle,FALLING);

  power_read.enableMeasurePower();
  power_read.selectMeasureCurrentOrVoltage(VOLTAGE);
  power_read.startMeasure();
  
  Serial.begin(115200);
  Serial.println("---------- SmartTimer v0.1 ----------");
  
  //File System Init
  SPIFFS.begin();
  //SPIFFS.format();
  // List files
  Serial.println("Files in SPIFFS");
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {    
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
	}

  // Load configuration
  if(!load_config()) {
    Serial.println("Couldn't load config");
  }

  wifiMode = EEPROM.read(0x01);
  if (wifiMode == 0x01) {
    Serial.print("WiFi STA mode setup ");
    Serial.println(config.ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.ssid.c_str(), config.password.c_str());
    Serial.print("MQTT Client setup ");
    Serial.print(config.mqtt_server);
    Serial.print(":");
    Serial.println(config.mqtt_port.toInt());
    if (config.mqtt_secure == "on") {
      Serial.println("Using SSL");
      mqttClient.setClient(wifiClientSecure);
    } else {
      Serial.println("Not using SSL");
      mqttClient.setClient(wifiClient);
    }
    mqttClient.setServer(config.mqtt_server.c_str(), config.mqtt_port.toInt());
    mqttClient.setCallback(callback);
  } else {
    flipper.attach(1.0, flip);
    EEPROM.write(0x01, 0x01);
    EEPROM.commit();
    Serial.print("WiFi AP mode setup ");
    String ssid = "SmartTimer-" + String(ESP.getChipId());
    Serial.println(ssid);
    WiFi.mode(WIFI_STA);
    WiFi.softAP(ssid.c_str(), "password");
    // HTTP server setup
    serverInit();
  }

  Serial.println("END Setup");
  longPress = millis();
}
 
void loop(void){
  if (reset) {
    detachInterrupt(digitalPinToInterrupt(BUTTON));
    flipper.detach();
    digitalWrite(LED, LOW);
    Serial.println("Restart!");
    delay(5000);
    ESP.restart();
  }
  if (wifiMode == 0x01) {
    // STA mode
    if (digitalRead(BUTTON) != LOW){
      longPress = millis();
    }
    if (millis() > (longPress + 2000)){
      EEPROM.write(0x01, 0x00);
      EEPROM.commit();
      reset = true;
    }
    timedTask();
    if (MQTT_connect()) {
      mqttClient.loop();
      flipper.detach();
      digitalWrite(LED, HIGH);
      if (pub) {
        Serial.print("Updating MQTT state ... ");
        String topicString = config.mqtt_topic + "/status";
        topicString.toCharArray(topic_buff, topicString.length()+1);
        if (digitalRead(RELAY)) {
          pub = mqttClient.publish(topic_buff,"ON",true);
        } else {
          pub = mqttClient.publish(topic_buff,"OFF",true);
        }
        if (pub) {
          Serial.print("OK");
        } else {
          Serial.print("ERROR");
        }
        Serial.println();
        pub = false;
      }
    }
  } else {
    // AP mode
    if (digitalRead(BUTTON) != LOW){
      longPress = millis();
    }
    if (millis() > (longPress + 2000)){
      EEPROM.write(0x01, 0x01);
      EEPROM.commit();
      reset = true;
    }
    server.handleClient(); // Handle Web server requests
  }
}

bool MQTT_connect() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi ...");
    flipper.detach();
    flipper.attach(0.3, flip);
    firstConnect = true;
    delay(500);
    return false;
  }

  if (firstConnect){
    Serial.print("Connected IP ");
    Serial.println(WiFi.localIP());
    firstConnect = false;
  }

  // Exit if already connected.
  if (mqttClient.connected()) { // connect will return 0 for connected
    return true;
  }

  flipper.detach();
  flipper.attach(0.1, flip);
  
  Serial.print("Connecting to MQTT broker as ");
  String clientId = "SmartTimer-" + String(ESP.getChipId()) + "-";
  clientId += String(random(ESP.getCycleCount()), HEX);
  clientId.toCharArray(client_buff, clientId.length()+1);
  Serial.println(clientId);  
  
  boolean conn;

  if (config.mqtt_user.length() > 0 && config.mqtt_psw.length() > 0){
    Serial.println("User " + config.mqtt_user);
    conn = mqttClient.connect(client_buff, config.mqtt_user.c_str(), config.mqtt_psw.c_str());
  } else {
    conn = mqttClient.connect(client_buff);
  }
  
  if (!conn) {
    int state = mqttClient.state();
    switch (state) {
      case MQTT_CONNECTION_TIMEOUT: Serial.println("The server didn't respond within the keepalive time"); break;
      case MQTT_CONNECTION_LOST: Serial.println("The network connection was broken"); break;
      case MQTT_CONNECT_FAILED: Serial.println("The network connection failed"); break;
      case MQTT_DISCONNECTED: Serial.println("The client is disconnected cleanly"); break;
      case MQTT_CONNECTED: Serial.println("The cient is connected"); break;
      case MQTT_CONNECT_BAD_PROTOCOL: Serial.println("The server doesn't support the requested version of MQTT"); break;
      case MQTT_CONNECT_BAD_CLIENT_ID: Serial.println("The server rejected the client identifier"); break;
      case MQTT_CONNECT_UNAVAILABLE: Serial.println("The server was unable to accept the connection"); break;
      case MQTT_CONNECT_BAD_CREDENTIALS: Serial.println("The username/password were rejected"); break;
      case MQTT_CONNECT_UNAUTHORIZED: Serial.println("The client was not authorized to connect"); break;
    }
    mqttClient.disconnect();
    return false;
  }

  String topicString = config.mqtt_topic + "/control";
  topicString.toCharArray(topic_buff, topicString.length()+1);  
  mqttClient.subscribe(topic_buff);
  
  return true;
}

void timedTask() {
  if ((millis() > TTasks + (kPwrUpdFreq*1000)) || (millis() < TTasks)) { 
    TTasks = millis();
    getPower();
  }
}

void getPower() {
  Serial.print("Reading data ... ");
  double power = power_read.getPower();
  yield();
  double voltage = power_read.getVoltage();
  yield();
  if (isnan(power_read.getPower() || power_read.getVoltage())) {
    Serial.println("ERROR");
    return;
  }
  Serial.println("OK");
  
  String pubString = "{\"power\":" + String(power) + "}";
  pubString.toCharArray(message_buff, pubString.length()+1);
  Serial.println(pubString);

  if (mqttClient.connected()){
    Serial.print("Updating MQTT data ... ");
    String topicString = config.mqtt_topic + "/power";
    topicString.toCharArray(topic_buff, topicString.length()+1);
    if (mqttClient.publish(topic_buff, message_buff)) {
      Serial.println("OK");
    } else {
      Serial.println("ERROR");
    }
  }

  if(WiFi.status() == WL_CONNECTED && config.oem_server.length() > 0 && config.oem_node.length() > 0 && config.oem_key.length() > 0) {
    Serial.print("Updating OEM data ... ");
    String url = "/input/post?json=" + pubString + "&node="+ config.oem_node + "&apikey=" + config.oem_key;
    if (!oemClient.connect(config.oem_server.c_str(), 443)) {
      Serial.println("Connection ERROR");
    } else {
      oemClient.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + config.oem_server + "\r\n" + "Connection: close\r\n\r\n");
      Serial.println("Connection OK");
    }  
  }
}
