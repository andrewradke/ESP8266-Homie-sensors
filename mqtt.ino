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
  
    if (mqttClient.publish(pubTopic, output, retain))
      watchdog = millis();    // feed the watchdog
  }
}


void mqttLog(String data) {
  mqttSend(String(FPSTR(mqttstr_log)), data, false);
  printMessage(logString, true);
  if (use_syslog) {
    syslog.appName(app_name);
    syslog.log(LOG_INFO, data);
  }
}


void mqttConnect() {
  if ( mqtt_tls && (! ca_cert_okay) ) {
    return;   // We can't do much if we are meant to use TLS and haven't been able to load the CA certificate
  }
  strncpy_P (app_name, app_name_mqtt, sizeof(app_name_mqtt) );
  syslog.appName(app_name);
  // Loop until we're reconnected
  if (!mqttClient.connected() && configured) {
    // Attempt to connect

    tmpString = String(baseTopic) + String(FPSTR(mqttstr_online));
    tmpString.toCharArray(pubTopic, OUT_STR_MAX);
    tmpString = String(FPSTR(str_false));
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
      logString = "Connected";
      printMessage(logString, true);
      if (use_syslog) {
        syslog.log(LOG_INFO, logString);
      }

      if (mqtt_tls) {
        // Verify validity of server's certificate before doing anything else
        logString = "server certificate ";
        if (espSecureClient.verifyCertChain(mqtt_server)) {
          logString = logString + "verified";
          tlsOkay = true;
        } else {
          logString = "ERROR: " + logString + "verification failed! Rebooting...";
          tlsOkay = false;
        }
        printMessage(logString, true);
        if (use_syslog) {
          syslog.log(LOG_INFO, logString);
        }
  
        if (! tlsOkay) {
          delay(1000);
          // reconnecting will cause certificate verification to fail so reboot to start clean
          ESP.restart();
          delay(5000);
      
          while (1);   // Do NOT send any other message!!! The certificate is invalid and it could be a security risk
        }
      }

      // Publush $online as true with pubTopic still correct from will above
      mqttSend(String(FPSTR(mqttstr_online)), String(FPSTR(str_true)), true);

      char subTopic[40];
      tmpString = String(baseTopic) + String(FPSTR(mqttstr_ota)) + String(FPSTR(str_command));
      logString = String("Subscribing to " + tmpString);
      tmpString.toCharArray(subTopic, 40);

      printMessage(logString, true);
      if (use_syslog) {
        syslog.log(LOG_INFO, logString);
      }
      mqttClient.subscribe(subTopic);

    } else {
      logString = "Failed to connect to MQTT, rc=" + String(mqttClient.state());
      printMessage(logString, true);
      if (use_syslog) {
        syslog.log(LOG_INFO, logString);
      }
    }
  }
}

/// The MQTT callback function for received messages
void mqttCallback(char* subTopic, byte* payload, unsigned int length) {
  strncpy_P (app_name, app_name_mqtt, sizeof(app_name_mqtt) );
  syslog.appName(app_name);

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
    printMessage(logString, true);
    if (use_syslog) {
      syslog.log(LOG_INFO, logString);
    }
    mqttSend(String(FPSTR(mqttstr_online)), String(FPSTR(str_false)), true);
    ESP.restart();
  } else {
    logString = String("UNKNOWN command: " + key);
    strncpy_P (app_name, app_name_mqtt, sizeof(app_name_mqtt) );
    mqttLog(logString);
    return;
  }
}
