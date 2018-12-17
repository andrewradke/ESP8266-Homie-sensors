#if FWTYPE == 16      // esp8266-depth-sensor

const char str_pressureRange[]  = "pressureRange";
const char str_depthOffset[]    = "depthOffset";

const char str_voltageVolts[]   = "voltage/volts";
const char str_pressurePsi[]    = "pressure/psi";
const char str_depthMetre[]     = "depth/metres";

void sensorSetup() {}

void  printSensorConfig(const String &cfgStr) {
  mqttSend(String(cfgStr + str_pressureRange), String(pressureRange), true);  
  mqttSend(String(cfgStr + str_depthOffset),   String(depthOffset),   true);  
}

void sensorImportJSON(JsonObject& json) {
  if (json[str_pressureRange].is<float>()) {
    pressureRange = json[str_pressureRange];
  }
  if (json[str_depthOffset].is<float>()) {
    depthOffset   = json[str_depthOffset];
  }
}

void sensorExportJSON(JsonObject& json) {
  json[str_pressureRange] = pressureRange;
  json[str_depthOffset]   = depthOffset;
}

bool sensorUpdateConfig(const String &key, const String &value) {
  if ( key == str_pressureRange ) {
    pressureRange = value.toFloat();
  } else if ( key == str_depthOffset ) {
    depthOffset = value.toFloat();
  } else {
    return false;
  }
  return true;
}


bool sensorRunAction(const String &key, const String &value) {
  return false;
}

void calcData() {
}

void sendData() {
  voltage = analogRead(A0);
  voltage *= .004887586;   // Convert 1023 to 5V

  if ( voltage > 4.95 ) {
    logString = ("ERROR: Sensor possibly shorted to 5V.");
    mqttLog(app_name_sensors, logString);
  } else {
    pressure = (voltage - 0.5) / 4 * pressureRange;
    depth    = pressure / 1.4579;                     // 1.4579psi/metre
    depth    += depthOffset;                          // Allow readings to be moved up or down depnding on the level the sensor is sitting in the water
    mqttSend(String(str_depthMetre),     String(depth),        false);
    mqttSend(String(str_voltageVolts),   String(voltage),      false);
    mqttSend(String(str_pressurePsi),    String(pressure),     false);
  }
}

String httpSensorData() {
  String httpData = tableStart;

  httpData += trStart + F("Depth:") + tdBreak;
  httpData += String(depth) + " m";
  httpData += trEnd;

  httpData += trStart + F("Pressure:") + tdBreak;
  httpData += String(pressure) + " psi";
  httpData += trEnd;


  httpData += tableEnd;
  return httpData;
}

String httpSensorSetup() {
  String httpData;
  httpData += trStart + F("Depth offset (m):") + tdBreak + htmlInput(str_text, str_depthOffset,   String(depthOffset))   + trEnd;
  httpData += trStart + F("Pressure range:")   + tdBreak + htmlInput(str_text, str_pressureRange, String(pressureRange)) + trEnd;
  return httpData;
}

String httpSensorConfig() {
  if (httpServer.hasArg(str_pressureRange)) {
    tmpString = String(pressureRange);
    if (httpServer.arg(str_pressureRange) != tmpString) {
      pressureRange = httpServer.arg(str_pressureRange).toFloat();
    }
  }
  if (httpServer.hasArg(str_depthOffset)) {
    tmpString = String(depthOffset);
    if (httpServer.arg(str_depthOffset) != tmpString) {
      depthOffset = httpServer.arg(str_depthOffset).toFloat();
    }
  }
}

void sensorMqttCallback(char* topic, byte* payload, unsigned int length) {}
void sensorMqttSubs() {}

#endif
