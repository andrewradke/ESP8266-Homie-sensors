#if FWTYPE == 14      // esp8266-pump-controller

// Need to add a way to signal pump priming

const char str_pressureRange[]  = "pressureRange";
const char str_pulsesPerLitre[] = "pulsesPerLitre";
const char str_cutoffLow[]      = "cutoffLow";
const char str_cutoffHigh[]     = "cutoffHigh";
const char str_pressureOn[]     = "pressureOn";
const char str_pressureOff[]    = "pressureOff";
const char str_flowMin[]        = "flowMin";
const char str_pumpCheckDelay[] = "pumpCheckDelay";

const char str_pump_Power[]      = "pump/power";
const char str_pump_Cutoff[]     = "cutoff/bool";
const char str_voltage_Volts[]   = "voltage/volts";
const char str_pressure_Psi[]    = "pressure/psi";
const char str_counter_Counter[] = "counter/counter";
const char str_litres_Counter[]  = "litres/counter";
const char str_lph_Lph[]         = "lph/lph";

const char str_pumpOn[]         = "on";
const char str_pumpOff[]        = "off";
const char str_pumpCutoff[]     = "cutoff";
const char str_Pump_[]          = "Pump ";
const char str_duePressure[]    = " due to pressure ";
const char str_dueFlow[]        = " due to flow ";


void sensorSetup() {
  pinMode(PULSE_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PULSE_PIN), flow, RISING);         // Setup Interrupt
  sei();                                                                   // Enable interrupts

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, pumpState);                                      // Set the initial state of the relay to off
}

void printSensorConfig(const String &cfgStr) {
  mqttSend(String(cfgStr + str_pressureRange),  String(pressureRange), true);
  mqttSend(String(cfgStr + str_pulsesPerLitre), String(pulsesPerLitre), true);
  mqttSend(String(cfgStr + str_cutoffLow),      String(cutoffLow), true);
  mqttSend(String(cfgStr + str_cutoffHigh),     String(cutoffHigh), true);
  mqttSend(String(cfgStr + str_pressureOn),     String(pressureOn), true);
  mqttSend(String(cfgStr + str_pressureOff),    String(pressureOff), true);
  mqttSend(String(cfgStr + str_flowMin),        String(flowMin), true);
  mqttSend(String(cfgStr + str_pumpCheckDelay), String(pumpCheckDelay), true);
}

void sensorImportJSON(JsonObject& json) {
  if (json[str_pressureRange].is<float>()) {
    pressureRange = json[str_pressureRange];
  }
  if (json[str_pulsesPerLitre].is<float>()) {
    pulsesPerLitre = json[str_pulsesPerLitre];
  }
  if (json[str_cutoffLow].is<float>()) {
    cutoffLow = json[str_cutoffLow];
  }
  if (json[str_cutoffHigh].is<float>()) {
    cutoffHigh = json[str_cutoffHigh];
  }
  if (json[str_pressureOn].is<float>()) {
    pressureOn = json[str_pressureOn];
  }
  if (json[str_pressureOff].is<float>()) {
    pressureOff = json[str_pressureOff];
  }
  if (json[str_flowMin].is<int>()) {
    flowMin = json[str_flowMin];
  }
  if (json[str_pumpCheckDelay].is<int>()) {
    pumpCheckDelay = json[str_pumpCheckDelay];
  }
}

void sensorExportJSON(JsonObject& json) {
  json[str_pressureRange]  = pressureRange;
  json[str_pulsesPerLitre] = pulsesPerLitre;
  json[str_cutoffLow]      = cutoffLow;
  json[str_cutoffHigh]     = cutoffHigh;
  json[str_pressureOn]     = pressureOn;
  json[str_pressureOff]    = pressureOff;
  json[str_flowMin]        = flowMin;
  json[str_pumpCheckDelay] = pumpCheckDelay;
}

bool sensorUpdateConfig(const String &key, const String &value) {
  if ( key == str_pressureRange ) {
    pressureRange = value.toFloat();
  } else if ( key == str_pulsesPerLitre ) {
    pulsesPerLitre = value.toFloat();
  } else if ( key == str_cutoffLow ) {
    cutoffLow = value.toInt();
  } else if ( key == str_cutoffHigh ) {
    cutoffHigh = value.toInt();
  } else if ( key == str_pressureOn ) {
    pressureOn = value.toInt();
  } else if ( key == str_pressureOff ) {
    pressureOff = value.toInt();
  } else if ( key == str_flowMin ) {
    flowMin = value.toInt();
  } else if ( key == str_pumpCheckDelay ) {
    pumpCheckDelay = value.toInt();
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
  if ( currentTime >= (floopTime + 1000) ) {
    floopTime = currentTime;                      // Updates floopTime
    flow_hz = flow_counter - flow_hz;
    // Pulse frequency (Hz) = 7.5 Q, Q is flow rate in L/min. (Results in +/- 3% range)
    l_hour = flow_hz * flowrate;                  // Pulse frequency x 60 min / PULSENUM = flow rate in L/hour
    flow_hz = flow_counter;                       // Reset counter
    litres = flow_counter / litrerate;            // get the total litres so far    

#ifdef USESSD1306
    display.clearDisplay();
    char text[7];
    dtostrf(pressure, 6, 1, text);
    display.setCursor(0, 0);
    display.setTextSize(3);
    display.print(text);
    display.setCursor(103, 15);
    display.setTextSize(1);
    display.print(" psi");

    if (pumpCutoff) {
      display.setCursor(0, 27);
      display.setTextSize(3);
      display.print("CUTOFF");
    } else {
      dtostrf(l_hour, 6, 0, text);
      display.setCursor(0, 27);
      display.setTextSize(3);
      display.print(text);
      display.setCursor(103, 42);
      display.setTextSize(1);
      display.print(" L/h");

      if (pumpState) {
        display.setCursor(0, 47);
        display.setTextSize(2);
        display.print(str_Pump_);
        display.print(str_pumpOn);
      }
    }

    display.display();
    prevDisplay = millis();
#endif

    if ( (pumpState) && ( (millis() / 1000) - pumpTimer < pumpCheckDelay ) ) {     // If we are in the pumpCheckDelay period send data every second
      sendData();
      mqttTime = currentTime;                      // Updates mqttTime
    }


  }
  if ( currentTime >= (cloopTime + 50) ) {
    voltage = analogRead(A0);
    // 0.004887585533 gives 4.75V
    // 0.005144826877 should give 5V
    voltage *= 0.005144826877;   // Convert 1023 to 5V
    pressure = (voltage - 0.5) / 4 * pressureRange;

    cloopTime = currentTime;                      // Updates cloopTime

    if (pumpCutoff) {
      // Check for pump cutoff reset, don't bother with interrupts or debouncing
      if ( digitalRead(CONFIG_PIN) == LOW ) {
        logString = String(str_Pump_) + String(str_pumpCutoff) + " reset";
        logMessage(app_name_sensors, logString, false);
        pumpCutoff = false;
        pumpControl(true);
      }
    } else {

      if ( (! pumpState) && (pressure <= pressureOn) ) {                                                                // If the pressure is low enough and the pump isn't running turn on even if it's below the cutoff pressure (we'll cutoff soon if it doesn't increase)
        logString = String(str_Pump_) + String(str_pumpOn) + String(str_duePressure) + "< " + String(pressureOn);
        logMessage(app_name_sensors, logString, false);
        pumpControl(true);
      } else if ( (pumpState) && ( (millis() / 1000) - pumpTimer > pumpCheckDelay ) && ( pressure < cutoffLow ) ) {     // Pump pressure too low. Cutoff if pumpCheckDelay exceeded
        logString = String(str_Pump_) + String(str_pumpCutoff) + String(str_duePressure) + "< " + String(cutoffLow);
        logMessage(app_name_sensors, logString, false);
        pumpCutoff = true;
        pumpControl(false);
      } else if (pumpState && (pressure > cutoffHigh) ) {                                                               // Pump pressure too high. Cutoff immediately, no delay
        logString = String(str_Pump_) + String(str_pumpCutoff) + String(str_duePressure) + "> " + String(cutoffHigh);
        logMessage(app_name_sensors, logString, false);
        pumpCutoff = true;
        pumpControl(false);
      } else if ( (pumpState) && ( (millis() / 1000) - pumpTimer > pumpCheckDelay ) && ( l_hour < flowMin ) ) {         // Pump is on but flow rate is too low. Turn off if pumpCheckDelay exceeded
        logString = String(str_Pump_) + String(str_pumpOff) + String(str_dueFlow) + "< " + String(flowMin);
        logMessage(app_name_sensors, logString, false);
        pumpControl(false);
      } else if ( (pumpState) && (pressure >= pressureOff) ) {                                                          // Pump is on but pressure is high enough to turn off.
        logString = String(str_Pump_) + String(str_pumpOff) + String(str_duePressure) + "> " + String(pressureOff);
        logMessage(app_name_sensors, logString, false);
        pumpControl(false);
      }
  
      if ( pumpState != pumpPrevState ) {           // Send the state *immediately* if it has changed, it can be sent again later without problems
        mqttSend(String(str_pump_Power), String(pumpState), false);  
        pumpPrevState = pumpState;
        pumpTimer = millis()/1000;
        sendData();
        mqttTime = currentTime;                      // Updates mqttTime
      }
    }
  }
}

void sendData() {
//  mqttSend(String(str_voltage_Volts),   String(voltage),      false);
  mqttSend(String(str_pressure_Psi),    String(pressure),     false);
//  mqttSend(String(str_counter_Counter), String(flow_counter), false);
  mqttSend(String(str_litres_Counter),  String(litres),       false);
  mqttSend(String(str_lph_Lph),         String(l_hour),       false);
  mqttSend(String(str_pump_Power),      String(pumpState),    false);
  mqttSend(String(str_pump_Cutoff),     String(pumpCutoff),   false);
}

void flow () {                     // Interrupt function
  flow_counter++;
}

String httpSensorData() {
  String httpData = tableStart;

//  httpData += trStart + F("Voltage:") + tdBreak;
//  httpData += String(voltage) + " V";
//  httpData += trEnd;

  httpData += trStart + F("Pressure:") + tdBreak;
  httpData += String(pressure) + " psi";
  httpData += trEnd;

//  httpData += trStart + F("Flow counter:") + tdBreak;
//  httpData += String(flow_counter);
//  httpData += trEnd;

  httpData += trStart + F("Flow rate:") + tdBreak;
  httpData += String(l_hour) + " L/h";
  httpData += trEnd;

  httpData += trStart + F("Total litres:") + tdBreak;
  httpData += String(litres) + " L";
  httpData += trEnd;

  httpData += trStart + F("Pump:") + tdBreak;
  if (pumpState) {
    httpData += String(F("<span style='color:green'>")) + String(str_pumpOn) + F("</span>");
  } else {
    httpData += String(str_pumpOff);
  }
  httpData += trEnd;

  httpData += trStart + F("Cutoff:") + tdBreak;
  if (pumpCutoff) {
    httpData += String(F("<span style='color:red'>")) + String(str_true) + F("</span>");
  } else {
    httpData += String(str_false);
  }
  httpData += trEnd;

  httpData += tableEnd;
  return httpData;
}

String httpSensorSetup() {
  String httpData;
  httpData += trStart + F("Pressure range:")               + tdBreak + htmlInput(str_text, str_pressureRange,  String(pressureRange))  + trEnd;
  httpData += trStart + F("Pulses/litre:")                 + tdBreak + htmlInput(str_text, str_pulsesPerLitre, String(pulsesPerLitre)) + trEnd;
  httpData += trStart + F("<b>Pressures:</b>") + tdBreak + trEnd;
  httpData += trStart + F("Cutout minimum:")               + tdBreak + htmlInput(str_text, str_cutoffLow,      String(cutoffLow))    + trEnd;
  httpData += trStart + F("Cutout maximum:")               + tdBreak + htmlInput(str_text, str_cutoffHigh,     String(cutoffHigh))    + trEnd;
  httpData += trStart + F("Pressure on:")                  + tdBreak + htmlInput(str_text, str_pressureOn,     String(pressureOn))     + trEnd;
  httpData += trStart + F("Pressure off:")                 + tdBreak + htmlInput(str_text, str_pressureOff,    String(pressureOff))    + trEnd;
  httpData += trStart + F("Cutout flow - minimum (L/h):")  + tdBreak + htmlInput(str_text, str_flowMin,        String(flowMin))        + trEnd;
  httpData += trStart + F("Startup check delay (s):")      + tdBreak + htmlInput(str_text, str_pumpCheckDelay, String(pumpCheckDelay)) + trEnd;
  return httpData;
}

String httpSensorConfig() {
  if (httpServer.hasArg(str_pressureRange)) {
    tmpString = String(pressureRange);
    if (httpServer.arg(str_pressureRange) != tmpString) {
      pressureRange = httpServer.arg(str_pressureRange).toFloat();
    }
  }
  if (httpServer.hasArg(str_pulsesPerLitre)) {
    tmpString = String(pulsesPerLitre);
    if (httpServer.arg(str_pulsesPerLitre) != tmpString) {
      pulsesPerLitre = httpServer.arg(str_pulsesPerLitre).toFloat();
    }
  }
  if (httpServer.hasArg(str_cutoffLow)) {
    tmpString = String(cutoffLow);
    if (httpServer.arg(str_cutoffLow) != tmpString) {
      cutoffLow = httpServer.arg(str_cutoffLow).toInt();
    }
  }
  if (httpServer.hasArg(str_cutoffHigh)) {
    tmpString = String(cutoffHigh);
    if (httpServer.arg(str_cutoffHigh) != tmpString) {
      cutoffHigh = httpServer.arg(str_cutoffHigh).toInt();
    }
  }
  if (httpServer.hasArg(str_pressureOn)) {
    tmpString = String(pressureOn);
    if (httpServer.arg(str_pressureOn) != tmpString) {
      pressureOn = httpServer.arg(str_pressureOn).toInt();
    }
  }
  if (httpServer.hasArg(str_pressureOff)) {
    tmpString = String(pressureOff);
    if (httpServer.arg(str_pressureOff) != tmpString) {
      pressureOff = httpServer.arg(str_pressureOff).toInt();
    }
  }
  if (httpServer.hasArg(str_flowMin)) {
    tmpString = String(flowMin);
    if (httpServer.arg(str_flowMin) != tmpString) {
      flowMin = httpServer.arg(str_flowMin).toInt();
    }
  }
  if (httpServer.hasArg(str_pumpCheckDelay)) {
    tmpString = String(pumpCheckDelay);
    if (httpServer.arg(str_pumpCheckDelay) != tmpString) {
      pumpCheckDelay = httpServer.arg(str_pumpCheckDelay).toInt();
    }
  }
}

void sensorMqttCallback(char* topic, byte* payload, unsigned int length) {
  char data[length];
  char checkTopic[50];

  strncpy(data, (char*)payload, length);
  data[length] = '\0';

//  logString = "MQTT topic: '" + String(topic) + ", data: " + String(data);
//  logMessage(app_name_mqtt, logString, false);

  baseTopic.toCharArray(checkTopic, 50);
  strcat_P(checkTopic, PSTR("pump/power/set"));
  if ( strcmp(topic, checkTopic) == 0 ) {
    logString = String(F("Set pump "));
    if ( ( length == 1 and data[0] == '1') or (length == 2 && strncasecmp(data,str_pumpOn, 2) == 0 ) ) {   // Only turn on for a '1' or 'on'
      logString += String(str_pumpOn);
      logMessage(app_name_mqtt, logString, false);
      pumpControl(true);
    } else {                                                                                         // For EVERYTHING ELSE turn off
      logString += String(str_pumpOff);
      logMessage(app_name_mqtt, logString, false);
      pumpControl(false);
    }
  } else {
    baseTopic.toCharArray(checkTopic, 50);
    strcat_P(checkTopic, PSTR("pump/cutoff/set"));
    if ( strcmp(topic, checkTopic) == 0 ) {
      logString = String(F("Set cutoff "));
      if ( ( length == 1 and data[0] == '1') or (length == 2 && strncasecmp(data,str_pumpOn, 2) == 0 ) ) {   // Only turn on for a '1' or 'on'
        logString += String(str_pumpOn);
        logMessage(app_name_mqtt, logString, false);
        pumpCutoff = true;
        pumpControl(false);
      } else {                                                                                         // For EVERYTHING ELSE turn off
        logString += String(str_pumpOff);
        logMessage(app_name_mqtt, logString, false);
        pumpCutoff = false;
      }
    } 
  }
}

void pumpControl(bool newState) {
  if ( newState && pumpCutoff ) {                // We won't turn on if the cutoff is active
    logString = F("Pump cutoff active. Not turning on.");
    logMessage(app_name_sensors, logString, false);
  } else {                                    // Otherwise turn on or off as requested
    if (pumpState != newState) {
      pumpState     = newState;
      pumpPrevState = pumpState;
      pumpTimer     = millis()/1000;
      digitalWrite(RELAY_PIN, pumpState);
    } else {
      logString = F("Pump already ");
      if (pumpState) {
        logString += String(str_pumpOn);
      } else {
        logString += String(str_pumpOff);
      }
      logMessage(app_name_sensors, logString, false);
    }
  }
  mqttSend(String(str_pump_Power), String(pumpState), false);
}

void sensorMqttSubs() {
  char extraSubTopic[50];
  baseTopic.toCharArray(extraSubTopic, 50);
  strcat_P(extraSubTopic, PSTR("pump/power/set"));
  logString = String(F("Subscribing to ")) + String(extraSubTopic);
  logMessage(app_name_mqtt, logString, false);
  mqttClient.subscribe(extraSubTopic);

  baseTopic.toCharArray(extraSubTopic, 50);
  strcat_P(extraSubTopic, PSTR("pump/cutoff/set"));
  logString = String(F("Subscribing to ")) + String(extraSubTopic);
  logMessage(app_name_mqtt, logString, false);
  mqttClient.subscribe(extraSubTopic);
}


#endif
