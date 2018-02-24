#if FWTYPE == 4      // esp8266-depth-sensor

void sensorSetup() {
}

void  printSensorConfig(String cfgStr) {
  mqttSend(String(cfgStr + "maxdepth"), String(maxdepth), true);
}

void sensorImportJSON(JsonObject& json) {
  if (json["maxdepth"].is<int>()) {
    maxdepth     = json["maxdepth"];
  }
}

void sensorExportJSON(JsonObject& json) {
  json["maxdepth"] = maxdepth;
}

bool sensorUpdateConfig(String key, String value) {
  if ( key == "maxdepth" ) {
    maxdepth = value.toInt();
  } else {
    return false;
  }
  return true;
}

bool sensorRunAction(String key, String value) {
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
#ifdef DEBUG
    dmesg();
    Serial.println("No valid ping result returned.");
#endif
    mqttTime = currentTime - (1000 * (mqtt_interval - 1 ) ); // only wait 1 second before trying again when measurement failed
  }
}

String httpSensorData() {
  String httpData = "<table>";
  String trStart = "<tr><td>";
  String tdBreak = "</td><td>";
  String trEnd   = "</td></tr>";

  httpData += trStart + "Distance:" + tdBreak;
  if ( uS != 0 ) {
    httpData += String(distance) + " cm";
  } else {
    httpData += "-";
  }
  httpData += trEnd;

  httpData += trStart + "Depth:" + tdBreak;
  if ( uS != 0 ) {
    httpData += String(depth) + " cm";
  } else {
    httpData += "-";
  }
  httpData += trEnd;

  httpData += "</table>";
  return httpData;
}

#endif

