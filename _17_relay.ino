#if FWTYPE == 17      // esp8266-relay

const char str_relay_Power[] = "relay/power";

const char str_relayOn[]     = "on";
const char str_relayOff[]    = "off";
const char str_Relay_[]      = "Relay ";


void sensorSetup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, relayState);  // Set the initial state of the relay to off
}

void printSensorConfig(const String &cfgStr) {}
void sensorImportJSON(JsonObject& json) {}
void sensorExportJSON(JsonObject& json) {}
bool sensorUpdateConfig(const String &key, const String &value) { return false; }
bool sensorRunAction(const String &key, const String &value) { return false; }
void calcData() {}

void sendData() {
  mqttSend(String(str_relay_Power), String(relayState), false);
}

String httpSensorData() {
  String httpData = tableStart;
  httpData += trStart + F("Relay:") + tdBreak;
  if (relayState) {
    httpData += String(F("<span style='color:green'>")) + String(str_relayOn) + F("</span>");
  } else {
    httpData += String(str_relayOff);
  }
  httpData += trEnd;
  httpData += tableEnd;
  return httpData;
}

String httpSensorSetup() { return String(""); }

String httpSensorConfig() {}

void sensorMqttCallback(char* topic, byte* payload, unsigned int length) {
  char data[length];
  char checkTopic[50];

  strncpy(data, (char*)payload, length);
  data[length] = '\0';

  baseTopic.toCharArray(checkTopic, 50);
  strcat_P(checkTopic, PSTR("relay/power/set"));
  if ( strcmp(topic, checkTopic) == 0 ) {
    logString = String(F("Set relay "));
    if ( ( length == 1 and data[0] == '1') or (length == 2 && strncasecmp(data,str_relayOn, 2) == 0 ) ) {   // Only turn on for a '1' or 'on'
      logString += String(str_relayOn);
      logMessage(app_name_sensors, logString, false);
      relayControl(true);
    } else {                                                                                         // For EVERYTHING ELSE turn off
      logString += String(str_relayOff);
      logMessage(app_name_sensors, logString, false);
      relayControl(false);
    }
  }
}

void relayControl(bool newState) {
  if (relayState != newState) {
    relayState     = newState;
    digitalWrite(RELAY_PIN, relayState);
  } else {
    logString = F("Relay already ");
    if (relayState) {
      logString += String(str_relayOn);
    } else {
      logString += String(str_relayOff);
    }
    logMessage(app_name_sensors, logString, false);
  }
  mqttSend(String(str_relay_Power), String(relayState), true);
}

void sensorMqttSubs() {
  char extraSubTopic[50];
  baseTopic.toCharArray(extraSubTopic, 50);
  strcat_P(extraSubTopic, PSTR("relay/power/set"));
  logString = String(F("Subscribing to ")) + String(extraSubTopic);
  logMessage(app_name_mqtt, logString, false);
  mqttClient.subscribe(extraSubTopic);
}


#endif
