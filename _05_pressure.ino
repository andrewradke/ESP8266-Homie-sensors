#if FWTYPE == 5      // esp8266-pressure-sensors

void sensorSetup() {
  ads1115.begin();
  tmpString = String("");
  for (byte i = 0; i < 4; i = i + 1) {
    if ( pressureRanges[i] != 0 ) {
      if (tmpString.length() != 0 ) {
        tmpString = String(tmpString + ',');
      }
      tmpString = String(tmpString + "pressure");
      tmpString = String(tmpString + i);
      tmpString = String(tmpString + ":pressure");
    }
  }
  tmpString.toCharArray(nodes, 100);
}

void  printSensorConfig(const String &cfgStr) {
  mqttSend(String(cfgStr + "pressurerange1"), String(pressureRanges[0]), true);
  mqttSend(String(cfgStr + "pressurerange2"), String(pressureRanges[1]), true);
  mqttSend(String(cfgStr + "pressurerange3"), String(pressureRanges[2]), true);
  mqttSend(String(cfgStr + "pressurerange4"), String(pressureRanges[3]), true);
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

bool sensorUpdateConfig(const String &key, const String &value) {
  if ( key == "pressurerange1" ) {
    pressureRanges[0] = value.toFloat();
  } else if ( key == "pressurerange2" ) {
    pressureRanges[1] = value.toFloat();
  } else if ( key == "pressurerange3" ) {
    pressureRanges[2] = value.toFloat();
  } else if ( key == "pressurerange4" ) {
    pressureRanges[3] = value.toFloat();
  } else {
    return false;
  }
  return true;
}

bool sensorRunAction(const String &key, const String &value) {
  return false;
}

void calcData() {}

void sendData() {
  float adsA, pressure;
  for (byte i = 0; i < 4; i = i + 1) {
    if ( pressureRanges[i] != 0 ) {
      adsA = ads1115.readADC_SingleEnded(i) * 0.0001875;
#ifdef DEBUG
      tmpString = String("analog");
      tmpString = String(tmpString + i);
      mqttSend(String(tmpString + "/voltage"), String(adsA), false);
#endif
      tmpString = String("pressure");
      tmpString = String(tmpString + i);
      pressure = (adsA - 0.5) / 4 * pressureRanges[i];
      mqttSend(String(tmpString + "/pressure"), String(pressure), false);
    }
  }
}

String httpSensorData() {
  String httpData = tableStart;

  float adsA, pressure;
  for (byte i = 0; i < 4; i = i + 1) {
    if ( pressureRanges[i] != 0 ) {
      adsA = ads1115.readADC_SingleEnded(i) * 0.0001875;

      httpData += trStart + "voltage " + String(i+1) + ":" + tdBreak;
      httpData += String(adsA) + " V";
      httpData += trEnd;

      pressure = (adsA - 0.5) / 4 * pressureRanges[i];

      httpData += trStart + "pressure " + String(i+1) + ":" + tdBreak;
      httpData += String(pressure) + " psi";
      httpData += trEnd;
    }
  }

  httpData += tableEnd;
  return httpData;
}

String httpSensorSetup() {
  String httpData;
  httpData += trStart + "Pressure ranges:" + tdBreak + trEnd;
  for (byte i = 0; i < 4; i = i + 1) {
    httpData += trStart + String(i+1) + ":" + tdBreak + htmlInput("text", "pressure" + String(i), String(pressureRanges[i])) + trEnd;
  }
  return httpData;
}

String httpSensorConfig() {
  for (byte i = 0; i < 4; i = i + 1) {
    if (httpServer.hasArg("pressure" + String(i))) {
      tmpString = String(pressureRanges[i]);
      if (httpServer.arg("pressure" + String(i)) != tmpString) {
        pressureRanges[i] = httpServer.arg("pressure" + String(i)).toFloat();
      }
    }
  }
}

void sensorMqttCallback(char* topic, byte* payload, unsigned int length) {}
void sensorMqttSubs() {}

#endif
