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
  if ( switchOpen != switchPrevOpen ) {           // Send the state *immediately* if it has changed, it can be sent again later without problems
    if (switchOpen) {
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


String httpSensorData() {
  String httpData = "<table>";
  String trStart = "<tr><td>";
  String tdBreak = "</td><td>";
  String trEnd   = "</td></tr>";

  httpData += trStart + "Switch:" + tdBreak;
  if (switchOpen) {
    httpData += "open";
  } else {
    httpData += "closed";
  }
  httpData += trEnd;

  httpData += "</table>";
  return httpData;
}

String httpSensorSetup() {
  String httpData;
  httpData += trStart + "No config items." + tdBreak + trEnd;
  return httpData;
}

String httpSensorConfig() {
}

#endif