#if FWTYPE == 3      // esp8266-switch

void sensorSetup() {
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  switchCheck();                             // Get the initial state for the switch
  attachInterrupt(digitalPinToInterrupt(SWITCH_PIN), switchCheck, CHANGE); // Setup Interrupt
  sei();                                    // Enable interrupts
}

void  printSensorConfig(const String &cfgStr) {
  return;
}

void sensorImportJSON(JsonObject& json) {}
void sensorExportJSON(JsonObject& json) {}

bool sensorUpdateConfig(const String &key, const String &value) {
  return false;
}

bool sensorRunAction(const String &key, const String &value) {
  return false;
}

void calcData() {
  switchCheck();                                  // This is driven by interrupts but this will catch any state change that may have been missed
  if ( switchOpen != switchPrevOpen ) {           // Send the state *immediately* if it has changed, it can be sent again later without problems
    if (switchOpen) {
      tmpString = String(switchOpenNoun);
    } else {
      tmpString = String(switchClosedNoun);
    }
    mqttSend(String(switchNodeVerb), tmpString, false);
    switchPrevOpen = switchOpen;
  }
}

void sendData() {
  if (switchOpen) {
    tmpString = String(switchOpenNoun);
  } else {
    tmpString = String(switchClosedNoun);
  }
  mqttSend(String(switchNodeVerb), tmpString, false);
}

void switchCheck () {               // Interrupt function
  switchOpen = digitalRead(SWITCH_PIN);
}


String httpSensorData() {
  String httpData = tableStart;
  httpData += trStart + "Switch:" + tdBreak;
  if (switchOpen) {
    httpData += String(switchOpenNoun);
  } else {
    httpData += String(switchClosedNoun);
  }
  httpData += trEnd;
  httpData += tableEnd;
  return httpData;
}

String httpSensorSetup() {
  String httpData;
  httpData += trStart + "No config items." + tdBreak + trEnd;
  return httpData;
}

String httpSensorConfig() {}
void sensorMqttCallback(char* topic, byte* payload, unsigned int length) {}
void sensorMqttSubs() {}

#endif
