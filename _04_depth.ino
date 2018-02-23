#if FWTYPE == 4      // esp8266-depth-sensor

void sensorSetup() {
  return;
}

void  printSensorConfig(String cfgStr) {
  return;
}

void sensorImportJSON(JsonObject& json) {
}

void sensorExportJSON(JsonObject& json) {
}

bool sensorUpdateConfig(String key, String value) {
  return false;
}

bool sensorRunAction(String key, String value) {
  return false;
}

void calcData() {
  uS = sonar.ping_median(10);   // Send 10 pings, get ping time in microseconds (uS).
}

void sendData() {
  if ( uS != 0 ) {
    mqttSend(String("distance/cm"), String(uS / US_ROUNDTRIP_CM), false);
  } else {
#ifdef DEBUG
    dmesg();
    Serial.println("No valid ping result returned.");
#endif
    mqttTime = currentTime - (1000 * (mqtt_interval - 1 ) );                       // only wait 1 second before trying again when measurement failed
  }
}

#endif

