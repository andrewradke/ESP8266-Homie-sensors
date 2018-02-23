#if FWTYPE == 3      // esp8266-switch

void sensorSetup() {
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  switchCheck();                             // Get the initial state for the switch
  attachInterrupt(digitalPinToInterrupt(SWITCH_PIN), switchCheck, RISING); // Setup Interrupt
  sei();                                    // Enable interrupts
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
  switchCheck();                                  // This is driven by interrupts but this will catch any state change that may have been missed
  if ( gateOpen != gatePrevOpen ) {               // Send the state *immediately* if it has changed, it can be sent again later without problems
    if (gateOpen) {
      tmpString = "open";
    } else {
      tmpString = "closed";
    }
    mqttSend(String("switch/switch"), tmpString, false);
    switchPrevOpen = switchOpen;
  }
}

void sendData() {
  if (switchOpen) {
    tmpString = "open";
  } else {
    tmpString = "closed";
  }
  mqttSend(String("switch/switch"), tmpString, false);
}

void switchCheck () {               // Interrupt function
  switchOpen = digitalRead(SWITCH_PIN);
}

#endif
