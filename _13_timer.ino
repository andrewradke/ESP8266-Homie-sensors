#if FWTYPE == 13      // Unused. Template ONLY.

void sensorSetup() {
  if (! mqtt_tls) {   // if we aren't using TLS for MQTT we won't be using NTP so we need to set it up here
    // Synchronize time useing SNTP. This is necessary to verify that
    // the TLS certificates offered by the server are currently valid.
    logString = F("Setting time using SNTP. Time zone: ");
    logString += String(tzoffset);
    logMessage(app_name_sys, logString, false);

    configTime(tzoffset * 3600, 0, ntp_server1, ntp_server2);
    time_t now = time(nullptr);
    while (now < tzoffset * 3600 * 2) {
      delay(500);
      Serial.print(str_dot);
      now = time(nullptr);
    }
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    logString = String(asctime(&timeinfo));
    logString.trim();
    logMessage(app_name_sys, logString, true);
  }
  for (byte i = 0; i < NUM_TIMERS; i++) {
    timers[i][0] = 0;
    timers[i][1] = 0;
    timers[i][2] = 0;
  }
  pinMode(relay_pin, OUTPUT);     // Initialize the relay pin as an output
  digitalWrite(relay_pin, LOW);   // Turn the relay off
}

void  printSensorConfig(const String &cfgStr) {
  mqttSend(String(cfgStr + "relay_pin"), String(relay_pin), true);
}

void sensorImportJSON(JsonObject& json) {
  if (json["relay_pin"].is<float>()) {
    relay_pin = json["relay_pin"];
  }
}

void sensorExportJSON(JsonObject& json) {
  json["relay_pin"] = relay_pin;
}

bool sensorUpdateConfig(const String &key, const String &value) {
// return true if any config item matches, false if none match
  if ( key == "relay_pin" ) {
    relay_pin = value.toInt();
  } else {
    return false;
  }
  return true;
}

bool sensorRunAction(const String &key, const String &value) {
// return true if any action matches, false if none match
  return false;
}


void sensorMqttSubs() {
  char extraSubTopic[50];
  for (byte i = 0; i < NUM_TIMERS; i++) {
    //homie/Test/timer_0/timer/set
    tmpString = String(baseTopic) + String(F("timer_")) + String(i) + String(F("/timer/set"));

    tmpString.toCharArray(extraSubTopic, 50);

    logString = String(F("Subscribing to ")) + tmpString;
    logMessage(app_name_mqtt, logString, false);

    mqttClient.subscribe(extraSubTopic);
  }
}

void sensorMqttCallback(char* topic, byte* payload, unsigned int length) {
  byte timer;
  String timerSubStr    = "";
  String timerDowStr    = "";
  String timerMinuteStr = "";
  String timerLengthStr = "";

  uint16_t timerDow    = 0;
  uint16_t timerMinute = 0;
  uint16_t timerLength = 0;

  // This only allows a maximum of 10 (0-9) timers
  timer = topic[strlen(topic)-11];   // Find the 11th char from the end which will be the timer number
  timer -= 48;                       // 0 is the 48th ASCII char, so subtract 48 and you get the actual number
  if (timer < 0 || timer > 9) {
    logString = F("Timer is not a value from 0 to 9: ");
    logString += String(topic[strlen(topic)-11]);
    logMessage(app_name_sys, logString, false);
    return;
  }

//  dmesg();
//  Serial.print("topic length: ");
//  Serial.println(strlen(topic));
  
  
/////// Check that the chars are only numbers and then convert them to the int as we go
  for (int i = 0; i < length; i++) {
    if ( ( (char)payload[i] == ',' ) && (timerMinuteStr == "") ) {
      if (timerDowStr == "")         timerDowStr    = timerSubStr;
      else if (timerMinuteStr == "") timerMinuteStr = timerSubStr;
      timerSubStr = "";
    } else if ( (char)payload[i] == ',' ) {
      timerLengthStr = timerSubStr;
      mqttCommand(timerDowStr, timerMinuteStr, timerLengthStr);
      timerSubStr    = "";
      timerDowStr    = "";
      timerMinuteStr = "";
      timerLengthStr = "";
    } else {
      timerSubStr = String(timerSubStr + String((char)payload[i]));
    }
  }
  if (timerDowStr == "")         timerDowStr    = timerSubStr;
  else if (timerMinuteStr == "") timerMinuteStr = timerSubStr;
  else                           timerLengthStr = timerSubStr;

  timerDow    = timerDowStr.toInt();
  timerMinute = timerMinuteStr.toInt();
  timerLength = timerLengthStr.toInt();

////////// Check the timerDow is valid
  if (timerMinute < 0 || timerMinute > 1440) {
    logString = F("Timer is not a valid minute of the day");
    logString += String(topic[strlen(topic)-11]);
    logMessage(app_name_sys, logString, false);
    return;
  }

  timers[timer][0] = timerDow;
  timers[timer][1] = timerMinute;
  timers[timer][2] = timerLength;

  dmesg();
  Serial.print("timer: ");
  Serial.println(timer);
  dmesg();
  Serial.print("dow: ");
  Serial.println(timerDow);
  dmesg();
  Serial.print("minute: ");
  Serial.println(timerMinute);
  dmesg();
  Serial.print("length: ");
  Serial.println(timerLength);
}

void calcData() {
  uint32_t currentTime = millis();
  if ( currentTime >= (lastTimer + 1000 ) ) {
    lastTimer = currentTime;                      // Updates lastTimer

    time_t now = time(nullptr);
    uint16_t thisMinute  = localtime(&now)->tm_hour * 60 + localtime(&now)->tm_min;
    byte dow = localtime(&now)->tm_wday;

    if ( thisMinute != lastMinute ) {
      lastMinute = thisMinute;
      // uint16_t timers[NUM_TIMERS][3];          // 3 numbers per timer: day of week bits, minute of day, length of timer
      for (byte i = 0; i < NUM_TIMERS; i++) {
        if ( bitRead(timers[i][0], dow) == 1 ) {  // if the bit for this day of the week is set
          if (thisMinute == timers[i][1]) {       // if the minute of the day matches
            logString = F("Timer ");
            logString += String(i+1) + F(" turning relay on for ") + String(timers[i][2]) + F(" seconds");
            logMessage(app_name_cfg, logString, false);
            relay_state = true;
            digitalWrite(relay_pin, relay_state); // Turn the relay ON
            mqttSend(String("relay/state"),   String(relay_state), false);
            relayOffTime = now + timers[i][2];
            break;                                // Don't process anymore timers since this one is a match
          }
        }
      }
    }
    if ( (relay_state) && ( now > relayOffTime ) ) {
      relay_state = false;
      digitalWrite(relay_pin, relay_state); // Turn the relay OFF
      logString = F("Turning relay off");
      logMessage(app_name_cfg, logString, false);
      mqttSend(String("relay/state"),   String(relay_state), false);
//      mqttSend(String("timers_") + String(i) + String("/state"),   String(relay_state), false);
    }
  }
}

void sendData() {
}

String httpSensorData() {
  String httpData = tableStart;
  httpData += trStart + "Relay:" + tdBreak;
  if (relay_state) {
    httpData += str_on;
  } else {
    httpData += str_off;
  }
  httpData += trEnd;
  httpData += tableEnd;
  return httpData;
}

String httpSensorSetup() {
  String httpData;
  httpData += trStart + F("Relay pin:") + tdBreak + htmlInput("text", "relay_pin", String(relay_pin)) + trEnd;
  return httpData;
}

String httpSensorConfig() {
  if (httpServer.hasArg("relay_pin")) {
    tmpString = String(relay_pin);
    if (httpServer.arg("relay_pin") != tmpString) {
      relay_pin = httpServer.arg("relay_pin").toInt();
      pinMode(relay_pin, OUTPUT);     // Initialize the relay pin as an output
      digitalWrite(relay_pin, LOW);   // Turn the relay off
    }
  }
}

#endif
