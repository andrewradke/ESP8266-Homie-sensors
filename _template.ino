#if FWTYPE == 0      // Unused. Template ONLY.
/* 
 * These are the minimum functions required. 
 * 
 * variables.h should also has a template section at the top to copy for includes, variables and WiFiManager config
 */


void sensorSetup() {
}

void  printSensorConfig(String cfgStr) {
  mqttSend(String(cfgStr + "sensor"), String(sensor_value), true);
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

#endif
