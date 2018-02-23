void mqttSend(String topic, String data, bool retain) {

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
  
    mqttClient.publish(pubTopic, output, retain);
  } else {
    logString = "not connected. Can't send data.";
    printMessage(logString, true);
#ifdef USESYSLOG
    syslog.appName(app_name);
    syslog.log(LOG_INFO, data);
#endif    
  }
}


void mqttLog(String data) {
  mqttSend(String("$log"), data, false);
  printMessage(logString, true);
#ifdef USESYSLOG
  syslog.appName(app_name);
  syslog.log(LOG_INFO, data);
#endif
}


void mqttConnect() {
  app_name = "MQTT";
  syslog.appName(app_name);
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    // Attempt to connect

    tmpString = String(baseTopic + "$online");
    tmpString.toCharArray(pubTopic, OUT_STR_MAX);
    tmpString = String("false");
    tmpString.toCharArray(output, OUT_STR_MAX);

    mqttClient.disconnect();

#ifdef USEMQTTAUTH
    if (mqttClient.connect(mqtt_name, mqtt_user, mqtt_passwd, pubTopic, 0, true, output)) {   // Connect with a will of $online set to false
#else
    if (mqttClient.connect(mqtt_name, pubTopic, 0, true, output)) {   // Connect with a will of $online set to false
#endif
      logString = "Connected";
      printMessage(logString, true);
#ifdef USESYSLOG
      syslog.log(LOG_INFO, logString);
#endif

#ifdef USETLS
      // Verify validity of server's certificate before doing anything else
      logString = "server certificate ";
      if (espClient.verifyCertChain(mqtt_server)) {
        logString = logString + "verified";
        tlsOkay = true;
      } else {
        logString = "ERROR: " + logString + "verification failed! Rebooting...";
        tlsOkay = false;
      }
      printMessage(logString, true);
#ifdef USESYSLOG
      syslog.log(LOG_INFO, logString);
#endif

      if (! tlsOkay) {
        delay(1000);
        // reconnecting will cause certificate verification to fail so reboot to start clean
        ESP.restart();
        delay(5000);
    
        while (1);   // Do NOT send any other message!!! The certificate is invalid and it could be a security risk
      }
#endif

      // Publush $online as true with pubTopic still correct from will above
      mqttSend(String("$online"), String("true"), true);

      char subTopic[40];
      tmpString = String(baseTopic + "$ota/command");
      logString = String("Subscribing to " + tmpString);
      tmpString.toCharArray(subTopic, 40);

      printMessage(logString, true);
#ifdef USESYSLOG
      syslog.log(LOG_INFO, logString);
#endif
      mqttClient.subscribe(subTopic);

    } else {
      logString = "Failed to connect to MQTT, rc=" + String(mqttClient.state()) + ". Trying again in 1 second";
      printMessage(logString, true);
#ifdef USESYSLOG
      syslog.log(LOG_INFO, logString);
#endif
      delay(1000);                // Wait 1 seconds before retrying
    }
  }

}

/// The MQTT callback function for received messages
void mqttCallback(char* subTopic, byte* payload, unsigned int length) {
  app_name = "MQTT";
#ifdef USESYSLOG
  syslog.appName(app_name);
#endif
  String mqttSubString = "";
  String mqttSubCmd    = "";
  String mqttSubKey    = "";
  String mqttSubValue  = "";
  char checkTopic[40];
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

void mqttCommand(String cmd, String key, String value) {
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
    if ( key == "config" ) {
      printConfig();
    } else if ( key == "system" ) {
      printSystem();
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
    printMessage(logString, true);
#ifdef USESYSLOG
    syslog.log(LOG_INFO, logString);
#endif
    mqttSend(String("$online"), String("false"), true);
    ESP.restart();
  } else {
    logString = String("UNKNOWN command: " + key);
    app_name = "MQTT";
    mqttLog(logString);
    return;
  }
}
