# Smart Timer

This project is made to run in the Sonoff POW hardware.

By default it starts an AP with the name "SmartTImer-XXXX" (chip ID) and the password is "password". Going to http://192.168.4.1 you will see the web configuration portal for WiFi and MQTT.

![portal](https://github.com/jebentancour/Smart-Timer/blob/master/portal.jpg)

By pressing the button (GPIO0) for 2 secconds it toggles between normal operation mode and configuration mode.

The softawe is based on [gmag11/FSBrowser](https://github.com/gmag11/FSBrowser) for the web configuration portal and [KmanOz/Sonoff-HomeAssistant](https://github.com/KmanOz/Sonoff-HomeAssistant) to get the power readings of the Sonoff POW.
