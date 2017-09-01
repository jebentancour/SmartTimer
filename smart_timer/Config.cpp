
#define CONFIG_FILE "config.cfg"

#include "Config.h"
#include "FSWebServer.h"
#include "DynamicData.h"

strConfig config;

// convert a single hex digit character to its integer value (from https://code.google.com/p/avr-netino/)
unsigned char h2int(char c)
{
	if (c >= '0' && c <= '9') {
		return((unsigned char)c - '0');
	}
	if (c >= 'a' && c <= 'f') {
		return((unsigned char)c - 'a' + 10);
	}
	if (c >= 'A' && c <= 'F') {
		return((unsigned char)c - 'A' + 10);
	}
	return(0);
}

boolean load_config() {
	File configFile = SPIFFS.open(CONFIG_FILE, "r");
	if (!configFile) {
		Serial.println("Failed to open config file");
    EEPROM.write(0x01, 0x00);
    EEPROM.commit();
		return false;
	}

	size_t size = configFile.size();
	if (size > 1024) {
		Serial.println("Config file size is too large");
		configFile.close();
    EEPROM.write(0x01, 0x00);
    EEPROM.commit();
		return false;
	}

	// Allocate a buffer to store contents of the file.
	std::unique_ptr<char[]> buf(new char[size]);

	// We don't use String here because ArduinoJson library requires the input
	// buffer to be mutable. If you don't use ArduinoJson, you may as well
	// use configFile.readString instead.
	configFile.readBytes(buf.get(), size);
	configFile.close();
	Serial.print("JSON file size: "); Serial.print(size); Serial.println(" bytes");

	StaticJsonBuffer<1024> jsonBuffer;
	JsonObject& json = jsonBuffer.parseObject(buf.get());

	if (!json.success()) {
		Serial.println("Failed to parse config file");
		return false;
	}
	String temp;
	json.prettyPrintTo(temp);
	Serial.println(temp);

	config.ssid = json["ssid"].asString();
	config.password = json["pass"].asString();
  config.mqtt_server = json["mqtt_server"].asString();
  config.mqtt_port = json["mqtt_port"].asString();
  config.mqtt_topic = json["mqtt_topic"].asString();
  config.mqtt_user = json["mqtt_user"].asString();
  config.mqtt_psw = json["mqtt_psw"].asString();
  config.mqtt_secure = json["mqtt_secure"].asString();
  config.oem_server = json["oem_server"].asString();
  config.oem_node = json["oem_node"].asString();
  config.oem_key = json["oem_key"].asString();
  delay(5);

	Serial.println("Data initialized.");
	Serial.print("WIFI_SSID: "); Serial.println(config.ssid);
	Serial.print("WIFI_PASS: "); Serial.println(config.password);
  Serial.print("MQTT_SERVER: "); Serial.println(config.mqtt_server);
  Serial.print("MQTT_PORT: "); Serial.println(config.mqtt_port);
  Serial.print("MQTT_TOPIC: "); Serial.println(config.mqtt_topic);
  Serial.print("MQTT_USER: "); Serial.println(config.mqtt_user);
  Serial.print("MQTT_PASS: "); Serial.println(config.mqtt_psw);
  Serial.print("MQTT_SECURE: "); Serial.println(config.mqtt_secure);
  Serial.print("OEM_SERVER: "); Serial.println(config.oem_server);
  Serial.print("OEM_NODE: "); Serial.println(config.oem_node);
  Serial.print("OEM_KEY: "); Serial.println(config.oem_key);
  delay(5);
  
	return true;
}

boolean save_config() {
	Serial.println("Saving config...");
	StaticJsonBuffer<1024> jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();
  
	json["ssid"] = config.ssid;
	json["pass"] = config.password;
  json["mqtt_server"] = config.mqtt_server;
  json["mqtt_port"] = config.mqtt_port;
  json["mqtt_topic"] = config.mqtt_topic;
  json["mqtt_user"] = config.mqtt_user;
  json["mqtt_psw"] = config.mqtt_psw;
  json["mqtt_secure"] = config.mqtt_secure;
  json["oem_server"] = config.oem_server;
  json["oem_node"] = config.oem_node;
  json["oem_key"] = config.oem_key;
			
	//TODO add AP data to html
	File configFile = SPIFFS.open(CONFIG_FILE, "w");
	if (!configFile) {
		Serial.println("Failed to open config file for writing");
		configFile.close();
		return false;
	}

	json.printTo(configFile);
	configFile.flush();
	configFile.close();
 
  String temp;
  json.prettyPrintTo(temp);
  Serial.println(temp);
  delay(10);
  
	return true;
}
