#if FWTYPE == 7      // esp8266-watermeter

void sensorSetup() {
#ifdef USEI2CADC
  Wire.begin();
#endif

  pinMode(PULSE_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PULSE_PIN), flow, RISING);         // Setup Interrupt
  sei();                                                                   // Enable interrupts

  pinMode(SWITCH_PIN, INPUT_PULLUP);
  switchCheck();                                                           // Get the initial state for the switch
  attachInterrupt(digitalPinToInterrupt(SWITCH_PIN), switchCheck, RISING); // Setup Interrupt
  sei();                                                                   // Enable interrupts
}

void  printSensorConfig(String cfgStr) {
  mqttSend(String(cfgStr + "pressurerange"), String(pressureRange), true);  
}

void sensorImportJSON(JsonObject& json) {
  if (json["pressureRange"].is<float>()) {
    pressureRange = json["pressureRange"];
  }
}

void sensorExportJSON(JsonObject& json) {
  json["pressureRange"] = pressureRange;
}

bool sensorUpdateConfig(String key, String value) {
  if ( key == "pressurerange" ) {
    pressureRange = value.toFloat();
  } else {
    return false;
  }
  return true;
}

bool sensorRunAction(String key, String value) {
  return false;
}

void calcData() {
  // Every second, get the current anolog reading and calculate and print litres/hour
  if ( currentTime >= (cloopTime + 1000) ) {
#ifdef USEI2CADC
    voltage = i2cADC.calcMillivolts() / 1000.0;
#else
    voltage = analogRead(A0);
    // 0.004887585533 gives 4.75V
    // 0.005144826877 should give 5V
    voltage *= 0.005144826877;   // Convert 1023 to 5V
#endif
    pressure = (voltage - 0.5) / 4 * pressureRange;

    cloopTime = currentTime;                      // Updates cloopTime
    flow_hz = flow_counter - flow_hz;
    // Pulse frequency (Hz) = 7.5 Q, Q is flow rate in L/min. (Results in +/- 3% range)
    l_hour = flow_hz * flowrate;                  // Pulse frequency x 60 min / PULSENUM = flow rate in L/hour
    flow_hz = flow_counter;                       // Reset counter
    litres = flow_counter / litrerate;            // get the total litres so far
  }

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
  mqttSend(String("voltage/volts"),   String(voltage),      false);
  mqttSend(String("pressure/psi"),    String(pressure),     false);
  mqttSend(String("counter/counter"), String(flow_counter), false);
  mqttSend(String("litres/counter"),  String(litres),       false);
  mqttSend(String("lph/lph"),         String(l_hour),       false);

  if (switchOpen) {
    tmpString = "open";
  } else {
    tmpString = "closed";
  }
  mqttSend(String("switch/switch"), tmpString, false);
}

void flow () {                     // Interrupt function
  flow_counter++;
}

void switchCheck () {               // Interrupt function
  switchOpen = digitalRead(SWITCH_PIN);
}

String httpSensorData() {
  String httpData = "<table>";
  String trStart = "<tr><td>";
  String tdBreak = "</td><td>";
  String trEnd   = "</td></tr>";

  httpData += trStart + "Voltage:" + tdBreak;
  httpData += String(voltage) + " V";
  httpData += trEnd;

  httpData += trStart + "Pressure:" + tdBreak;
  httpData += String(pressure) + " psi";
  httpData += trEnd;

  httpData += trStart + "Flow counter:" + tdBreak;
  httpData += String(flow_counter);
  httpData += trEnd;

  httpData += trStart + "Litres:" + tdBreak;
  httpData += String(litres) + " l";
  httpData += trEnd;

  httpData += trStart + "Flow rate:" + tdBreak;
  httpData += String(l_hour) + " lph";
  httpData += trEnd;

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
  httpData += trStart + "Pressure range:" + tdBreak + htmlInput("text", "pressureRange", String(pressureRange)) + trEnd;
  return httpData;
}

String httpSensorConfig() {
  if (httpServer.hasArg("pressureRange")) {
    tmpString = String(pressureRange);
    if (httpServer.arg("pressureRange") != tmpString) {
      pressureRange = httpServer.arg("pressureRange").toFloat();
    }
  }
}

#endif
