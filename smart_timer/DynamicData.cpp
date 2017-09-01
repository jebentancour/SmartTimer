
#include "DynamicData.h"
#include "Config.h"
#include "FSWebServer.h"
#include <StreamString.h>

String urldecode(String input){
  char c;
  String ret = "";

  for (byte t = 0; t<input.length(); t++){
    c = input[t];
    if (c == '+') c = ' ';
    if (c == '%') {
      t++;
      c = input[t];
      t++;
      c = (h2int(c) << 4) | h2int(input[t]);
    }
    ret.concat(c);
  }
  return ret;
}

void send_networks_scan_xml(){
	String networks = "<?xml version=\"1.0\"?><scan><found>";

	int n = WiFi.scanNetworks();

	if (n == 0)	{
		networks += "0</found>";
	}	else {
		networks += String(n) + "</found>";
		networks += "<networks>";
		for (int i = 0; i < n; ++i)	{
			int quality = 0;
			if (WiFi.RSSI(i) <= -100)	{
				quality = 0;
			}	else if (WiFi.RSSI(i) >= -50) {
				quality = 100;
			}	else {
				quality = 2 * (WiFi.RSSI(i) + 100);
			}
			networks += "<network><ssid>" + String(WiFi.SSID(i)) + "</ssid><quality>" + String(quality) + "</quality><encryption>" + String((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? "false" : "true") + "</encryption></network>";
		}
		networks += "</networks>";
	}

  networks += "</scan>";

	server.send(200, "text/xml", networks);
}

void send_configuration_values_xml(){

	String values = "<?xml version=\"1.0\"?><configuration>";

	values += "<ssid>" + (String)config.ssid + "</ssid>";
	values += "<psw>" + (String)config.password + "</psw>";
  values += "<mqtt_server>" + (String)config.mqtt_server + "</mqtt_server>";
  values += "<mqtt_port>" + (String)config.mqtt_port + "</mqtt_port>";
  values += "<mqtt_topic>" + (String)config.mqtt_topic + "</mqtt_topic>";
  values += "<mqtt_user>" + (String)config.mqtt_user + "</mqtt_user>";
  values += "<mqtt_psw>" + (String)config.mqtt_psw + "</mqtt_psw>";
  values += "<mqtt_secure>" + (String)config.mqtt_secure + "</mqtt_secure>";
  values += "<oem_server>" + (String)config.oem_server + "</oem_server>";
  values += "<oem_node>" + (String)config.oem_node + "</oem_node>";
  values += "<oem_key>" + (String)config.oem_key + "</oem_key>";

  values += "</configuration>";
	
	server.send(200, "text/xml", values);
}

void save_configuration(){
	if (server.args() > 0) { // Save Settings
		for (uint8_t i = 0; i < server.args(); i++) {
			Serial.printf("Arg %s: %s\n", server.argName(i).c_str(), server.arg(i).c_str());

			if (server.argName(i) == "ssid") { config.ssid = urldecode(server.arg(i));	continue; }
			if (server.argName(i) == "psw") {	config.password = urldecode(server.arg(i)); continue; }
      if (server.argName(i) == "mqtt_server") { config.mqtt_server = urldecode(server.arg(i)); continue; }
      if (server.argName(i) == "mqtt_port") { config.mqtt_port = urldecode(server.arg(i)); continue; }
      if (server.argName(i) == "mqtt_topic") { config.mqtt_topic = urldecode(server.arg(i)); continue; }
      if (server.argName(i) == "mqtt_user") { config.mqtt_user = urldecode(server.arg(i)); continue; }
      if (server.argName(i) == "mqtt_psw") { config.mqtt_psw = urldecode(server.arg(i)); continue; }
      if (server.argName(i) == "mqtt_secure") { config.mqtt_secure = urldecode(server.arg(i)); continue; }
      if (server.argName(i) == "oem_server") { config.oem_server = urldecode(server.arg(i)); continue; }
      if (server.argName(i) == "oem_node") { config.oem_node = urldecode(server.arg(i)); continue; }
      if (server.argName(i) == "oem_key") { config.oem_key = urldecode(server.arg(i)); continue; }

		}
		save_config();
		EEPROM.write(0x01, 0x01);
    EEPROM.commit();
	} else {
    Serial.println("Empty Args");
	}
  server.sendHeader("Location", String("/"), true);
  server.send (302, "text/plain", "");
}

void req_reset(){
  server.send(200, "text/xml", "<?xml version=\"1.0\"?><msg>Dispositivo reiniciado!</msg>");
  Serial.println("Restart!");
  delay(1000);
  ESP.restart();
}


