#if FWTYPE == 2      // esp8266-flow-counter

void sensorSetup() {
  pinMode(PULSE_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PULSE_PIN), flow, RISING); // Setup Interrupt
  sei();                                                           // Enable interrupts
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

void flow () {                     // Interrupt function
  flow_counter++;
}

#endif
