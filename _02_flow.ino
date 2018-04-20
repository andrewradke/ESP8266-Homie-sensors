#if FWTYPE == 2      // esp8266-flow-counter

void sensorSetup() {
  pinMode(PULSE_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PULSE_PIN), flow, RISING); // Setup Interrupt
  sei();                                                           // Enable interrupts
}

void  printSensorConfig(const String &cfgStr) {
  mqttSend(String(cfgStr + "pulsesPerLitre"), String(pulsesPerLitre), true);
}

void sensorImportJSON(JsonObject& json) {
  if (json["pulsesPerLitre"].is<float>()) {
    pulsesPerLitre     = json["pulsesPerLitre"];
    flowrate     = 60 / pulsesPerLitre;
    litrerate    = 60 * pulsesPerLitre;
  }
}

void sensorExportJSON(JsonObject& json) {
  json["pulsesPerLitre"] = pulsesPerLitre;
}

bool sensorUpdateConfig(const String &key, const String &value) {
  if ( key == "pulsesPerLitre" ) {
    pulsesPerLitre = value.toFloat();
  } else {
    return false;
  }
  return true;
}

bool sensorRunAction(const String &key, const String &value) {
  return false;
}

void calcData() {
  // Every second, calculate and print litres/hour
  if ( currentTime >= (cloopTime + 1000) ) {
    cloopTime = currentTime;                      // Updates cloopTime
    flow_hz = flow_counter - flow_hz;
    // Pulse frequency (Hz) = 7.5 Q, Q is flow rate in L/min. (Results in +/- 3% range)
    l_hour = flow_hz * flowrate;                  // Pulse frequency x 60 min / PULSENUM = flow rate in L/hour
    flow_hz = flow_counter;                       // Reset counter
    litres = flow_counter / litrerate;            // get the total litres so far
  }
}

void sendData() {
  mqttSend(String("counter/counter"), String(flow_counter), false);
  mqttSend(String("litres/counter"),  String(litres),       false);
  mqttSend(String("lph/lph"),         String(l_hour),       false);
}

String httpSensorData() {
  String httpData = tableStart;
  httpData += trStart + "Flow counter:" + tdBreak;
  httpData += String(flow_counter);
  httpData += trEnd;

  httpData += trStart + "Litres:" + tdBreak;
  httpData += String(litres) + " l";
  httpData += trEnd;

  httpData += trStart + "Flow rate:" + tdBreak;
  httpData += String(l_hour) + " lph";
  httpData += trEnd;

  httpData += tableEnd;
  return httpData;
}

String httpSensorSetup() {
  String httpData;
  httpData += trStart + "Pulses per litre:" + tdBreak + htmlInput("text",     "pulsesPerLitre", String(pulsesPerLitre)) + trEnd;
  return httpData;
}

String httpSensorConfig() {
  if (httpServer.hasArg("pulsesPerLitre")) {
    tmpString = String(pulsesPerLitre);
    if (httpServer.arg("pulsesPerLitre") != tmpString) {
      pulsesPerLitre = httpServer.arg("pulsesPerLitre").toFloat();
      flowrate     = 60 / pulsesPerLitre;
      litrerate    = 60 * pulsesPerLitre;
    }
  }
}

void flow () {                     // Interrupt function
  flow_counter++;
}

#endif
