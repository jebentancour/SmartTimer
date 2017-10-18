
#ifndef _FSWEBSERVER_h
#define _FSWEBSERVER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <FS.h>

extern ESP8266WebServer server;

String formatBytes(size_t bytes);
String getContentType(String filename);
bool handleFileRead(String path);
void handleFileList();
void serverInit();

#endif

