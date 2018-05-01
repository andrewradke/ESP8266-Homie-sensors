const String mqtt_rc_states[10]  = { "Connection timeout", "Connection lost", "Connect failed", "Disconnected", "Connected", "Connect: bad protocol", "Connect: bad client ID", "Connect: unavailable", "Connect: bad credentials", "Connect: unauthorised" };

void mqttSend(const String &topic, const String &data, bool retain) {

#ifdef DEBUG
  dmesg();
  Serial.print(topic);
  Serial.print(" ");
  Serial.println(data);
#endif

  if (mqttClient.connected()) {
    tmpString = String(baseTopic + topic);
    tmpString.toCharArray(pubTopic, OUT_STR_MAX);
  
    tmpString = String(data);
    tmpString.toCharArray(output, OUT_STR_MAX);
  
    if (mqttClient.publish(pubTopic, output, retain))
      watchdog = millis();    // feed the watchdog
  }
}


void mqttLog(const String &app_name, const String &message) {
  mqttSend(String(FPSTR(mqttstr_log)), message, false);
  logMessage(app_name, message, true);
}


void mqttConnect() {
  if ( mqtt_tls && (! ca_cert_okay) ) {
    return;   // We can't do much if we are meant to use TLS and haven't been able to load the CA certificate
  }

  // Loop until we're reconnected
  if (!mqttClient.connected() && configured) {
    // Attempt to connect

    tmpString = String(baseTopic) + String(FPSTR(mqttstr_online));
    tmpString.toCharArray(pubTopic, OUT_STR_MAX);
    tmpString = str_false;
    tmpString.toCharArray(output, OUT_STR_MAX);

    mqttClient.disconnect();

    bool mqttResult;
    // Connect with a will of $online set to false (from output set above)
    if (mqtt_auth) {
      mqttResult = mqttClient.connect(mqtt_name, mqtt_user, mqtt_passwd, pubTopic, 0, true, output);
    } else {
      mqttResult = mqttClient.connect(mqtt_name, pubTopic, 0, true, output);
    }

    if (mqttResult) {
      logString = str_connected;
      logMessage(app_name_mqtt, logString, true);

      if (mqtt_tls) {
        // Verify validity of server's certificate before doing anything else
        logString = F("server certificate ");
        if (espSecureClient.verifyCertChain(mqtt_server)) {
          logString = logString + F("verified");
          tlsOkay = true;
        } else {
          logString = String(F("ERROR: ")) + logString + F("verification failed! ") + str_rebooting;
          tlsOkay = false;
        }
        logMessage(app_name_mqtt, logString, false);
  
        if (! tlsOkay) {
          delay(1000);
          // reconnecting will cause certificate verification to fail so reboot to start clean
          ESP.restart();
          delay(5000);
      
          while (1);   // Do NOT send any other message!!! The certificate is invalid and it could be a security risk
        }
      }

      // Publush $online as true with pubTopic still correct from will above
      mqttSend(String(FPSTR(mqttstr_online)), str_true, true);

      char subTopic[40];
      tmpString = String(baseTopic) + String(FPSTR(mqttstr_ota)) + String(FPSTR(str_command));
      tmpString.toCharArray(subTopic, 40);

      logString = String(F("Subscribing to ")) + tmpString;
      logMessage(app_name_mqtt, logString, false);

      mqttClient.subscribe(subTopic);

    } else {
      logString = F("Failed to connect to MQTT, ");
      logString += mqtt_rc_states[mqttClient.state() + 4];    // The return codes start from -4 so we have to add four to get the correct array position
      logString += " (rc=" + String(mqttClient.state()) + ")";
      logMessage(app_name_mqtt, logString, false);
    }
  }
}

/// The MQTT callback function for received messages
void mqttCallback(char* subTopic, byte* payload, unsigned int length) {
  String mqttSubString = "";
  String mqttSubCmd    = "";
  String mqttSubKey    = "";
  String mqttSubValue  = "";
  char checkTopic[40];

  watchdog = millis();    // feed the watchdog

#ifdef DEBUG
  dmesg();
  Serial.print("Message arrived [");
  Serial.print(subTopic);
  Serial.print("] ");
  Serial.println("");
#endif

  for (int i = 0; i < length; i++) {
    if ( ( (char)payload[i] == ':' ) && (mqttSubKey == "") ) {
      if (mqttSubCmd == "")        mqttSubCmd   = mqttSubString;
      else if (mqttSubKey == "")   mqttSubKey   = mqttSubString;
      mqttSubString = "";
    } else if ( (char)payload[i] == ',' ) {
      mqttSubValue = mqttSubString;
      mqttCommand(mqttSubCmd, mqttSubKey, mqttSubValue);
      mqttSubString = "";
      mqttSubCmd    = "";
      mqttSubKey    = "";
      mqttSubValue  = "";
    } else {
      mqttSubString = String(mqttSubString + String((char)payload[i]));
    }
  }
  if (mqttSubCmd == "")        mqttSubCmd   = mqttSubString;
  else if (mqttSubKey == "")   mqttSubKey   = mqttSubString;
  else                         mqttSubValue = mqttSubString;

  mqttCommand(mqttSubCmd, mqttSubKey, mqttSubValue);
}

void mqttCommand(const String &cmd, const String &key, const String &value) {
#ifdef DEBUG
  dmesg();
  Serial.print("  cmd:  ");
  Serial.println(cmd);
  dmesg();
  Serial.print("  key:  ");
  Serial.println(key);
  dmesg();
  Serial.print("  value:");
  Serial.println(value);
  dmesg();
  Serial.println("----------");
#endif

  if ( cmd == "set" ) {
    if ( key == "interval" ) {
      mqtt_interval = value.toInt();
    }
  } else if ( cmd == "get" ) {
    if ( key == "system" ) {
      mqtt_send_systeminfo();
    }
  } else if ( cmd == "action" ) {
    runAction(key, value);
  } else if ( cmd == "update" ) {
    if ( key == "firmware" ) {
      updateFirmware(value);
    }
  } else if ( cmd == "config" ) {
    updateConfig(key, value);
  } else if ( cmd == "saveconfig" ) {
    saveConfig();
  } else if ( cmd == "savewifi" ) {
    WiFi.begin(ssid.c_str(), psk.c_str(), 0, NULL, false);
  } else if ( cmd == "reboot" ) {
    logString = "Received reboot command.";
    logMessage(app_name_mqtt, logString, true);

    mqttSend(String(FPSTR(mqttstr_online)), str_false, true);
    ESP.restart();
  } else {
    logString = String("UNKNOWN command: " + key);
    mqttLog(app_name_mqtt, logString);
    return;
  }
}
