#if FWTYPE == 4      // esp8266-depth-sensor

void sensorSetup() {}

void  printSensorConfig(const String &cfgStr) {
  mqttSend(String(cfgStr + "maxdepth"), String(maxdepth), true);
}

void sensorImportJSON(JsonObject& json) {
  if (json["maxdepth"].is<float>()) {
    maxdepth     = json["maxdepth"];
  }
}

void sensorExportJSON(JsonObject& json) {
  json["maxdepth"] = maxdepth;
}

bool sensorUpdateConfig(const String &key, const String &value) {
  if ( key == "maxdepth" ) {
    maxdepth = value.toFloat();
  } else {
    return false;
  }
  return true;
}

bool sensorRunAction(const String &key, const String &value) {
  return false;
}

void calcData() {
  uS = sonar.ping_median(readings);                          // Send readings pings, get ping time in microseconds (uS).
  distance = (uS / US_ROUNDTRIP_CM);                         // Calculate the distance in cm
  distance /= 100;                                           // Convert from cm to metres
  depth    = maxdepth - distance;
}

void sendData() {
  if ( uS != 0 ) {
    mqttSend(String("distance/m"), String(distance), false);
    mqttSend(String("depth/m"),    String(depth),    false);
  } else {
    logString = F("ERROR: No valid distance returned");
    mqttLog(app_name_sensors, logString);
    mqttSend(String("distance/m"), str_NaN, false);
    mqttSend(String("depth/m"),    str_NaN, false);
    mqttTime = currentTime - (1000 * (mqtt_interval - 1 ) ); // only wait 1 second before trying again when measurement failed
  }
}

String httpSensorData() {
  String httpData = tableStart;
  httpData += trStart + F("Distance:") + tdBreak;
  if ( uS != 0 ) {
    httpData += String(distance) + " m";
  } else {
    httpData += "-";
  }
  httpData += trEnd;

  httpData += trStart + F("Depth:") + tdBreak;
  if ( uS != 0 ) {
    httpData += String(depth) + " m";
  } else {
    httpData += "-";
  }
  httpData += trEnd;

  httpData += tableEnd;
  return httpData;
}

String httpSensorSetup() {
  String httpData;
  httpData += trStart + F("Max depth (m):") + tdBreak + htmlInput("text",     "maxdepth", String(maxdepth)) + trEnd;
  return httpData;
}

String httpSensorConfig() {
  if (httpServer.hasArg("maxdepth")) {
    tmpString = String(maxdepth);
    if (httpServer.arg("maxdepth") != tmpString) {
      maxdepth = httpServer.arg("maxdepth").toFloat();
    }
  }
}

void sensorMqttCallback(char* topic, byte* payload, unsigned int length) {}
void sensorMqttSubs() {}

#endif

