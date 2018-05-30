#if FWTYPE == 6      // esp8266-loadcell

void sensorSetup() {
  hx711.begin(PIN_SCALE_DOUT, PIN_SCALE_SCK);
}

void  printSensorConfig(const String &cfgStr) {
  mqttSend(String(cfgStr + "offset"), String(offset), true);
  mqttSend(String(cfgStr + "scale"), String(scale), true);
}

void sensorImportJSON(JsonObject& json) {
  if (json["offset"].is<int>()) {
    offset       = json["offset"];
  }
  if (json["scale"].is<float>()) {
    scale        = json["scale"];
  }
}

void sensorExportJSON(JsonObject& json) {
  json["offset"] = offset;
  json["scale"]  = scale;
}

bool sensorUpdateConfig(const String &key, const String &value) {
  if ( key == "offset" ) {
    offset = value.toInt();
  } else if ( key == "scale" ) {
    scale = value.toFloat();
  } else {
    return false;
  }
  return true;
}

bool sensorRunAction(const String &key, const String &value) {
  if ( ( key == "tare" ) || ( key == "rezero" ) ) {
    rezero();
  } else if ( ( key == "scale" ) || ( key == "calibrate" ) ) {
    calibrate();
  } else {
    return false;
  }
  return true;
}

void calcData() {
  weight = get_units(readings);
}

void sendData() {
  mqttSend(String("weight/grams"), String((int) (weight + 0.5)), false);              // Adding 0.5 will make sure it is rounded to the nearest integer
}

void rezero() {
  logString = "Re-zeroing";
  mqttLog(app_name_sensors, logString);

  tare(readings * 2);                       // reset the scale to 0

  mqttSend(String("$config/offset"), String(offset), true);
}


String httpSensorData() {
  String httpData = tableStart;
  httpData += trStart + "Weight:" + tdBreak;
  httpData += String(weight) + " g";
  httpData += trEnd;
  httpData += tableEnd;
  return httpData;
}

String httpSensorSetup() {
  String httpData;
  httpData += trStart + "offset:" + tdBreak + htmlInput("text",     "offset", String(offset)) + trEnd;
  httpData += trStart + "scale:"  + tdBreak + htmlInput("text",     "scale",  String(scale))  + trEnd;
  return httpData;
}

String httpSensorConfig() {
  if (httpServer.hasArg("offset")) {
    tmpString = String(offset);
    if (httpServer.arg("offset") != tmpString) {
      offset = httpServer.arg("offset").toFloat();
    }
  }
  if (httpServer.hasArg("scale")) {
    tmpString = String(scale);
    if (httpServer.arg("scale") != tmpString) {
      scale = httpServer.arg("scale").toFloat();
    }
  }
}

void sensorMqttCallback(char* topic, byte* payload, unsigned int length) {}
void sensorMqttSubs() {}



void calibrate() {
  logString = "Calibrating";
  mqttLog(app_name_sensors, logString);

  float unscaled = get_value(readings * 2);
  scale = unscaled / 100;

  mqttSend(String("$config/scale"), String(scale), true);
}

long read_average(byte times) {
  long sum = 0;
  for (byte i = 0; i < times; i++) {
    sum += hx711.read();
    yield();
  }
#ifdef DEBUG
  dmesg();
  Serial.print("average: ");
  Serial.println(sum / times);
#endif
  return sum / times;
}

long read_median(byte times) {
  unsigned int weights[times], last;
  uint32_t j, i = 0;
  weights[0] = 0;

  while (i < times) {
    last  = hx711.read();

    if (i > 0) {                         // Don't start sort till second measurement.
      for (j = i; j > 0 && weights[j - 1] < last; j--) // Insertion sort loop.
        weights[j] = weights[j - 1];     // Shift measurement array to correct position for sort insertion.
    } else j = 0;                        // First measurement is sort starting point.
    weights[j] = last;                   // Add last measurement to array in sorted position.
    i++;                                 // Move to next measurement.

    delay(10);                           // 10ms delay between each measurement
  }
  return (weights[times >> 1]);          // Return the measurement median.
}


long get_value(byte times) {
  return read_median(times) - offset;
}

float get_units(byte times) {
  return get_value(times) / scale;
}

void tare(byte times) {
  double sum = read_median(times);
  offset = sum;
}

#endif
