#if FWTYPE == 0      // Unused. Template ONLY.
/* 
 * These are the minimum functions required. 
 * 
 * variables.h should also has a template section at the top to copy for includes, variables and WiFiManager config
 */

void sensorSetup() {
}

void  printSensorConfig(String cfgStr) {
  mqttSend(String(cfgStr + "config_item"), String(config_item), true);
}

void sensorImportJSON(JsonObject& json) {
  if (json["config_item"].is<float>()) {
    config_item = json["config_item"];
  }
}

void sensorExportJSON(JsonObject& json) {
  json["config_item"] = config_item;
}

bool sensorUpdateConfig(String key, String value) {
// return true if any config item matches, false if none match
  if ( key == "config_item" ) {
    config_item = value.toFloat();
  } else {
    return false;
  }
  return true;
}

bool sensorRunAction(String key, String value) {
// return true if any action matches, false if none match
  return false;
}


void calcData() {
  sensor_value = 42;
}

void sendData() {
  mqttSend(String("sensor/number"),   String(sensor_value), false);
}

String httpSensorData() {
  String httpData = tableStart;
  httpData += trStart + "Sensor:" + tdBreak;
  httpData += String(sensor_value) + " units";
  httpData += trEnd;
  httpData += tableEnd;
  return httpData;
}

String httpSensorSetup() {
  String httpData;
  httpData += trStart + "Config item:" + tdBreak + htmlInput("text", "config_item", config_item) + trEnd;
  return httpData;
}

String httpSensorConfig() {
  if (httpServer.hasArg("config_item")) {
    tmpString = String(config_item);
    if (httpServer.arg("config_item") != tmpString) {
      config_item = httpServer.arg("config_item").toFloat();
    }
  }
}

#endif
