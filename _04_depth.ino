#if FWTYPE == 4      // esp8266-depth-sensor

void sensorSetup() {
}

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
  distance = (uS / US_ROUNDTRIP_CM) + 0.5;                   // Adding 0.5 will make sure it is rounded to the nearest integer
  depth    = maxdepth - distance;
}

void sendData() {
  if ( uS != 0 ) {
    mqttSend(String("distance/cm"), String(distance), false);
    mqttSend(String("depth/cm"),    String(depth),    false);
  } else {
    logString = F("ERROR: No valid distance returned");
    mqttLog(app_name_sensors, logString);
    mqttSend(String("distance/cm"), str_NaN, false);
    mqttSend(String("depth/cm"),    str_NaN, false);
    mqttTime = currentTime - (1000 * (mqtt_interval - 1 ) ); // only wait 1 second before trying again when measurement failed
  }
}

String httpSensorData() {
  String httpData = tableStart;
  httpData += trStart + F("Distance:") + tdBreak;
  if ( uS != 0 ) {
    httpData += String(distance) + " cm";
  } else {
    httpData += "-";
  }
  httpData += trEnd;

  httpData += trStart + F("Depth:") + tdBreak;
  if ( uS != 0 ) {
    httpData += String(depth) + " cm";
  } else {
    httpData += "-";
  }
  httpData += trEnd;

  httpData += tableEnd;
  return httpData;
}

String httpSensorSetup() {
  String httpData;
  httpData += trStart + F("Max depth (cm):") + tdBreak + htmlInput("text",     "maxdepth", String(maxdepth)) + trEnd;
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

#endif

