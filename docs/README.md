# ESP8266 Homie sensors

A ESP8266 firmware that includes support for a range of sensors sending to MQTT over TLS and following the Homie convention (version 1 at this stage).

The aim is to be as secure as possible (HTTPS for the config page is the only significant outstanding issue) and make it easy to add another sensor type in without having to ready any non-sensor specific code.

## Getting Started

This code is compiled through the Arduino IDE with ESP8266 support added.

### PrerequisitesG

Some extra libraries are required:

* ArduinoJson: https://github.com/bblanchon/ArduinoJson
* PubSubClient: http://pubsubclient.knolleary.net/
* Syslog: https://github.com/arcao/Syslog

If you wish to use an OLED display the following Adafruit libraries are also needed:

* Adafruit_GFX
* Adafruit_SSD1306

### Installing

Change the FWTYPE to match the sensor type being used. For example to use a flow counter in the main ESP8266_homie_unified.ino file set:

```
#define FWTYPE     2
```

After compiling and uploading the firmware to the ESP8266 a WiFi acess point will be available with the same name as the sensor type. The default password is "esp8266." without quotes. Once connected you can access it's web configuration page on http://192.168.4.1/ and set it to join your local WiFi network.

The default MQTT name of the sensor will be the same as it's firmware type name, e.g. esp8266-flow-counter. This can be changed on the Configuration page of the web interface.

Once the ESP8266 has been joined to you WiFi network it will be accessible via http://<name>.local/  or via it's IP address displayed in it's boot up logs via the serial interface.

## Future planned development

* Better code comments
* Documenting how to use the firmware
* Implement the more recent versions of the Homie Convention
* Further reduce RAM and flash usage by moving all strings to flash and making sure only single use strings are referenced using F()
* Find a reasonable way to support HTTPS for the web interface
* Add support for multiple DS18B20 temperature sensors using the newer homie conventions and reporting as homie/device ID/1wire-address/temperature
* Add more supported sensors

## Contributing

All contributions, ideas and critisism is welcome. :-)

## Authors

* **Andrew Radke**

## License

This project is licensed under the GNU General Public License Version 3 available at https://www.gnu.org/licenses/gpl-3.0.en.html

## Acknowledgements

* Marvin Roger for his work on The Homie Convention (https://github.com/homieiot/convention)
