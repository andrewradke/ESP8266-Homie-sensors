#if FWTYPE == 5      // esp8266-pressure-sensor

void sensorSetup() {
  ads1115.begin();
  int i;
  tmpString = String("");
  for (i = 0; i < 4; i = i + 1) {
    if ( pressureRanges[i] != 0 ) {
      if (tmpString.length() != 0 ) {
        tmpString = String(tmpString + ',');
      }
      tmpString = String(tmpString + "analog");
      tmpString = String(tmpString + i);
      tmpString = String(tmpString + ":pressure");
    }
  }
  tmpString.toCharArray(nodes, 100);
}

void  printSensorConfig(String cfgStr) {
  mqttSend(String(cfgStr + "pressurerange0"), String(pressureRanges[0]), true);
  mqttSend(String(cfgStr + "pressurerange1"), String(pressureRanges[1]), true);
  mqttSend(String(cfgStr + "pressurerange2"), String(pressureRanges[2]), true);
  mqttSend(String(cfgStr + "pressurerange3"), String(pressureRanges[3]), true);
}

void sensorImportJSON(JsonObject& json) {
  if (json["pressureRanges"].is<JsonArray&>()) {
    pressureRanges[0] = json["pressureRanges"][0];
    pressureRanges[1] = json["pressureRanges"][1];
    pressureRanges[2] = json["pressureRanges"][2];
    pressureRanges[3] = json["pressureRanges"][3];
  }
}

void sensorExportJSON(JsonObject& json) {
  JsonArray& json_pressureRanges = json.createNestedArray("pressureRanges");
  json_pressureRanges.add(pressureRanges[0]);
  json_pressureRanges.add(pressureRanges[1]);
  json_pressureRanges.add(pressureRanges[2]);
  json_pressureRanges.add(pressureRanges[3]);
}

bool sensorUpdateConfig(String key, String value) {
  if ( key == "pressurerange0" ) {
    pressureRanges[0] = value.toFloat();
  } else if ( key == "pressurerange1" ) {
    pressureRanges[1] = value.toFloat();
  } else if ( key == "pressurerange2" ) {
    pressureRanges[2] = value.toFloat();
  } else if ( key == "pressurerange3" ) {
    pressureRanges[3] = value.toFloat();
  } else {
    return false;
  }
  return true;
}

bool sensorRunAction(String key, String value) {
  return false;
}

void calcData() {
  return; // read the values when they are being sent
}

void sendData() {
  float adsA, pressure;
  int i;
  for (i = 0; i < 4; i = i + 1) {
    if ( pressureRanges[i] != 0 ) {
      tmpString = String("analog");
      tmpString = String(tmpString + i);
      adsA = ads1115.readADC_SingleEnded(i) * 0.0001875;
#ifdef DEBUG
      mqttSend(String(tmpString + "/voltage"), String(adsA), false);
#endif
      pressure = (adsA - 0.5) / 4 * pressureRanges[i];
      mqttSend(String(tmpString + "/pressure"), String(pressure), false);
    }
  }
}

#endif

