#if FWTYPE == 7      // esp8266-watermeter

const char str_pressureRange[]  = "pressureRange";
const char str_pulsesPerLitre[] = "pulsesPerLitre";

const char str_switchSwitch[]   = "switch/Switch";
const char str_voltageVolts[]   = "voltage/volts";
const char str_pressurePsi[]    = "pressure/psi";
const char str_counterCounter[] = "counter/counter";
const char str_litresCounter[]  = "litres/counter";
const char str_lphLph[]         = "lph/lph";


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

void  printSensorConfig(const String &cfgStr) {
  mqttSend(String(cfgStr) + String(str_pressureRange), String(pressureRange), true);  
}

void sensorImportJSON(JsonObject& json) {
  if (json[str_pressureRange].is<float>()) {
    pressureRange = json[str_pressureRange];
  }
}

void sensorExportJSON(JsonObject& json) {
  json[str_pressureRange] = pressureRange;
}

bool sensorUpdateConfig(const String &key, const String &value) {
  if ( key == str_pressureRange ) {
    pressureRange = value.toFloat();
  } else {
    return false;
  }
  return true;
}

bool sensorRunAction(const String &key, const String &value) {
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
//    if (switchOpen) {
//      tmpString = String(switchOpenNoun);
//    } else {
//      tmpString = String(switchClosedNoun);
//    }
    mqttSend(String(str_switchSwitch), String(switchOpen), false);
    switchPrevOpen = switchOpen;
  }
}

void sendData() {
  mqttSend(String(str_voltageVolts),   String(voltage),      false);
  mqttSend(String(str_pressurePsi),    String(pressure),     false);
  mqttSend(String(str_counterCounter), String(flow_counter), false);
  mqttSend(String(str_litresCounter),  String(litres),       false);
  mqttSend(String(str_lphLph),         String(l_hour),       false);
  mqttSend(String(str_switchSwitch),   String(switchOpen),   false);
}

void flow () {                     // Interrupt function
  flow_counter++;
}

void switchCheck () {               // Interrupt function
  switchOpen = digitalRead(SWITCH_PIN);
}

String httpSensorData() {
  String httpData = tableStart;

  httpData += trStart + F("Voltage:") + tdBreak;
  httpData += String(voltage) + " V";
  httpData += trEnd;

  httpData += trStart + F("Pressure:") + tdBreak;
  httpData += String(pressure) + " psi";
  httpData += trEnd;

  httpData += trStart + F("Flow counter:") + tdBreak;
  httpData += String(flow_counter);
  httpData += trEnd;

  httpData += trStart + F("Litres:") + tdBreak;
  httpData += String(litres) + " l";
  httpData += trEnd;

  httpData += trStart + F("Flow rate:") + tdBreak;
  httpData += String(l_hour) + " lph";
  httpData += trEnd;

  httpData += trStart + F("Switch:") + tdBreak;
  if (switchOpen) {
    tmpString = String(switchOpenNoun);
  } else {
    tmpString = String(switchClosedNoun);
  }
  httpData += tmpString;
  httpData += trEnd;

  httpData += tableEnd;
  return httpData;
}

String httpSensorSetup() {
  String httpData;
  httpData += trStart + F("Pressure range:") + tdBreak + htmlInput(str_text, str_pressureRange, String(pressureRange)) + trEnd;
  return httpData;
}

String httpSensorConfig() {
  if (httpServer.hasArg(str_pressureRange)) {
    tmpString = String(pressureRange);
    if (httpServer.arg(str_pressureRange) != tmpString) {
      pressureRange = httpServer.arg(str_pressureRange).toFloat();
    }
  }
}

void sensorMqttCallback(char* topic, byte* payload, unsigned int length) {}
void sensorMqttSubs() {}

#endif
