
#ifndef _CONFIG_h
#define _CONFIG_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

typedef struct {
	String ssid;
	String password;
  String mqtt_server;
  String mqtt_port;
  String mqtt_topic;
  String mqtt_user;
  String mqtt_psw;
  String mqtt_secure;
  String oem_server;
  String oem_node;
  String oem_key;
} strConfig;

extern strConfig config;

unsigned char h2int(char c);

boolean load_config();

boolean save_config();

#endif

