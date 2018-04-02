void printDone() {
  Serial.println(" done.");
}


void dmesg() {
  Serial.print('[');
  Serial.print(millis() / 1000);
  Serial.print("] ");
}


String IPtoString(IPAddress address) {
  return String(address[0]) + "." +
         String(address[1]) + "." +
         String(address[2]) + "." +
         String(address[3]);
}


void printMessage(String app_name, String message, bool oled) {
  dmesg();
  Serial.print(app_name);
  Serial.print(": ");
  Serial.println(message);
#ifdef USESSD1306
  if (oled) {
    message = String(app_name) + ": " + message;
    display.setTextSize(1);
    while ( message.length() ) {
      if ( displayLine >= ROWS ) {    // Move everything up one line.
        display.clearDisplay();
        displayLine = ROWS - 1;
        for (byte y=0; y<(ROWS-1); y++) {
          for (byte x=0; x<COLS; x++) {
            if ( y<(ROWS-1) )
              displayArray[y][x] = displayArray[y+1][x];
            else
              displayArray[y][x] = ' ';
          }
          display.setCursor(0, y*8);
          display.print(displayArray[y]);
        }
      }

      if ( message.length() <= COLS ) {
        message.toCharArray(displayArray[displayLine], message.length()+1);
        message = "";
      } else {
        bool foundBreak = false;
        for (byte x=COLS; x>0; x--) {
          if ( message.charAt(x-1) == ' ' or message.charAt(x-1) == '-' ) {
            foundBreak = true;
            message.toCharArray(displayArray[displayLine], x+1);    // include this character on this line
            message.remove(0, x);
            break;
          }
        }
        if ( ! foundBreak ) {
          message.toCharArray(displayArray[displayLine], COLS+1);    // one more character or the null terminator will swallow it
          message.remove(0, COLS);
        }
      }
      display.setCursor(0, displayLine*8);
      display.print(displayArray[displayLine]);
      displayLine++;
    }
    display.display();
    prevDisplay = millis();
  }
#endif
}

void logMessage(String app_name, String message, bool oled) {
  printMessage(app_name, logString, oled);

  if (use_syslog) {
    char appname[10];
    app_name.toCharArray(appname,10);
    syslog.appName(appname);
    syslog.log(LOG_INFO, logString);
    // The delay is needed to give time for each syslog packet to be sent. Otherwise a few of the later ones can end up being lost
    delay(1);
  }
}

void mqtt_send_systeminfo() {
  mqttSend(String(FPSTR(mqttstr_online)),       String(FPSTR(str_true)), true);
  mqttSend(String(FPSTR(mqttstr_fwname)),       String(fwname), true);
  mqttSend(String(FPSTR(mqttstr_fwversion)),    String(FWVERSION), true);
  mqttSend(String(FPSTR(mqttstr_name)),         String(mqtt_name), true);
  mqttSend(String(FPSTR(mqttstr_nodes)),        String(nodes), true);
  mqttSend(String(FPSTR(mqttstr_localip)),      IPtoString(WiFi.localIP()), true);
}



void saveConfig() {
  char* cfg_name;

  logString = F("Saving config");
  logMessage(app_name_cfg, logString, true);

  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();

  json[String(FPSTR(cfg_use_staticip))]  = use_staticip;

  JsonArray& json_ip = json.createNestedArray(String(FPSTR(cfg_ip_address)));
  json_ip.add(ip[0]);
  json_ip.add(ip[1]);
  json_ip.add(ip[2]);
  json_ip.add(ip[3]);
  JsonArray& json_subnet = json.createNestedArray(String(FPSTR(cfg_subnet)));
  json_subnet.add(subnet[0]);
  json_subnet.add(subnet[1]);
  json_subnet.add(subnet[2]);
  json_subnet.add(subnet[3]);
  JsonArray& json_gateway = json.createNestedArray(String(FPSTR(cfg_gateway)));
  json_gateway.add(gateway[0]);
  json_gateway.add(gateway[1]);
  json_gateway.add(gateway[2]);
  json_gateway.add(gateway[3]);
  JsonArray& json_dns = json.createNestedArray(String(FPSTR(cfg_dns_server)));
  json_dns.add(dns_ip[0]);
  json_dns.add(dns_ip[1]);
  json_dns.add(dns_ip[2]);
  json_dns.add(dns_ip[3]);

  json[String(FPSTR(cfg_ntp_server1))]   = ntp_server1;
  json[String(FPSTR(cfg_ntp_server2))]   = ntp_server2;

  json[String(FPSTR(cfg_mqtt_server))]   = mqtt_server;
  json[String(FPSTR(cfg_mqtt_port))]     = mqtt_port;
  json[String(FPSTR(cfg_mqtt_name))]     = mqtt_name;
  json[String(FPSTR(cfg_mqtt_tls))]      = mqtt_tls;
  json[String(FPSTR(cfg_mqtt_auth))]     = mqtt_auth;
  json[String(FPSTR(cfg_mqtt_user))]     = mqtt_user;
  json[String(FPSTR(cfg_mqtt_passwd))]   = mqtt_passwd;
  json[String(FPSTR(cfg_mqtt_interval))] = mqtt_interval;
  json[String(FPSTR(cfg_mqtt_watchdog))] = mqtt_watchdog;

  json[String(FPSTR(cfg_use_syslog))]    = use_syslog;
  json[String(FPSTR(cfg_host_name))]     = host_name;
  json[String(FPSTR(cfg_syslog_server))] = syslog_server;

  json[String(FPSTR(cfg_httpUser))]      = httpUser;
  json[String(FPSTR(cfg_httpPasswd))]    = httpPasswd;

  configured            = true;
  json[String(FPSTR(cfg_configured))]    = configured;

  sensorExportJSON(json);

  File configFile = SPIFFS.open(configfilename, "w");
  if (!configFile) {
    logString = F("Failed to open config file for writing.");
    logMessage(app_name_cfg, logString, true);
  }

#ifdef DEBUG
  dmesg();
  json.printTo(Serial);
  Serial.println();
#endif

  json.printTo(configFile);
  configFile.close();
}  //end saveConfig


void loadConfig() {
  //read configuration from FS json
  logString = F("Mounting FS...");
  logMessage(app_name_sys, logString, true);

  if (SPIFFS.begin()) {
    if (SPIFFS.exists(configfilename)) {
      File configFile = SPIFFS.open(configfilename, "r");
      if (configFile) {
        logString = F("Opened config file.");
        logMessage(app_name_cfg, logString, true);

        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
#ifdef DEBUG
        dmesg();
        json.prettyPrintTo(Serial);
        Serial.println();
#endif
        if (json.success()) {
          if (json[String(FPSTR(cfg_use_staticip))].is<bool>()) {
            use_staticip = json[String(FPSTR(cfg_use_staticip))];
          }
          if (json[String(FPSTR(cfg_ip_address))].is<JsonArray&>()) {
            ip[0]        = json[String(FPSTR(cfg_ip_address))][0];
            ip[1]        = json[String(FPSTR(cfg_ip_address))][1];
            ip[2]        = json[String(FPSTR(cfg_ip_address))][2];
            ip[3]        = json[String(FPSTR(cfg_ip_address))][3];
          }
          if (json[String(FPSTR(cfg_subnet))].is<JsonArray&>()) {
            subnet[0]    = json[String(FPSTR(cfg_subnet))][0];
            subnet[1]    = json[String(FPSTR(cfg_subnet))][1];
            subnet[2]    = json[String(FPSTR(cfg_subnet))][2];
            subnet[3]    = json[String(FPSTR(cfg_subnet))][3];
          }
          if (json[String(FPSTR(cfg_gateway))].is<JsonArray&>()) {
            gateway[0]   = json[String(FPSTR(cfg_gateway))][0];
            gateway[1]   = json[String(FPSTR(cfg_gateway))][1];
            gateway[2]   = json[String(FPSTR(cfg_gateway))][2];
            gateway[3]   = json[String(FPSTR(cfg_gateway))][3];
          }
          if (json[String(FPSTR(cfg_dns_server))].is<JsonArray&>()) {
            dns_ip[0]    = json[String(FPSTR(cfg_dns_server))][0];
            dns_ip[1]    = json[String(FPSTR(cfg_dns_server))][1];
            dns_ip[2]    = json[String(FPSTR(cfg_dns_server))][2];
            dns_ip[3]    = json[String(FPSTR(cfg_dns_server))][3];
          }

          if (json[String(FPSTR(cfg_ntp_server1))].is<const char*>()) {
            strcpy(ntp_server1, json[String(FPSTR(cfg_ntp_server1))]);
          }
          if (json[String(FPSTR(cfg_ntp_server2))].is<const char*>()) {
            strcpy(ntp_server2, json[String(FPSTR(cfg_ntp_server2))]);
          }


          if (json[String(FPSTR(cfg_mqtt_server))].is<const char*>()) {
            strcpy(mqtt_server,   json[String(FPSTR(cfg_mqtt_server))]);
          }
          if (json[String(FPSTR(cfg_mqtt_port))].is<const char*>()) {
            strcpy(mqtt_port,     json[String(FPSTR(cfg_mqtt_port))]);
          }
          if (json[String(FPSTR(cfg_mqtt_name))].is<const char*>()) {
            strcpy(mqtt_name,     json[String(FPSTR(cfg_mqtt_name))]);
          }
          if (json[String(FPSTR(cfg_mqtt_tls))].is<bool>()) {
            mqtt_tls = json[String(FPSTR(cfg_mqtt_tls))];
          }
          if (json[String(FPSTR(cfg_mqtt_auth))].is<bool>()) {
            mqtt_auth = json[String(FPSTR(cfg_mqtt_auth))];
          }
          if (json[String(FPSTR(cfg_mqtt_user))].is<const char*>()) {
            strcpy(mqtt_user,     json[String(FPSTR(cfg_mqtt_user))]);
          }
          if (json[String(FPSTR(cfg_mqtt_passwd))].is<const char*>()) {
            strcpy(mqtt_passwd,   json[String(FPSTR(cfg_mqtt_passwd))]);
          }
          if (json[String(FPSTR(cfg_mqtt_interval))].is<int>()) {
            mqtt_interval = json[String(FPSTR(cfg_mqtt_interval))];
          }
          if (json[String(FPSTR(cfg_mqtt_watchdog))].is<int>()) {
            mqtt_watchdog = json[String(FPSTR(cfg_mqtt_watchdog))];
          }


          if (json[String(FPSTR(cfg_use_syslog))].is<bool>()) {
            use_syslog = json[String(FPSTR(cfg_use_syslog))];
          }
          if (json[String(FPSTR(cfg_host_name))].is<const char*>()) {
            strcpy(host_name,     json[String(FPSTR(cfg_host_name))]);
          }
          if (json[String(FPSTR(cfg_syslog_server))].is<const char*>()) {
            strcpy(syslog_server, json[String(FPSTR(cfg_syslog_server))]);
          }

          if (json[String(FPSTR(cfg_httpUser))].is<const char*>()) {
            httpUser = json[String(FPSTR(cfg_httpUser))].as<String>();
          }
          if (json[String(FPSTR(cfg_httpPasswd))].is<const char*>()) {
            httpPasswd = json[String(FPSTR(cfg_httpPasswd))].as<String>();
          }

          if (json[String(FPSTR(cfg_configured))].is<bool>()) {
            configured = json[String(FPSTR(cfg_configured))];
          }

          strcpy(host_name, mqtt_name); // We use the MQTT name as the host name since there is little value in having both and it just adds another config item and potential confusion.

          sensorImportJSON(json);

          // The config file has been found, opened and is good json
          configLoaded = true;

        } else {
          logString = F("Corrupted json config file.");
          logMessage(app_name_cfg, logString, true);
        }
      } else {
        logString = F("Failed to open config file.");
        logMessage(app_name_cfg, logString, true);
      }
    } else {
      logString = F("No existing config file.");
      logMessage(app_name_cfg, logString, true);
    }
  } else {
    logString = F("Failed to mount FS.");
    logMessage(app_name_sys, logString, true);
  }
}  //end readConfig


void updateConfig(String key, String value) {
  if ( key == "ssid" ) {
    ssid = value;
  } else if ( key == "psk" ) {
    psk = value;

  } else if ( key == String(FPSTR(cfg_ip_address)) ) {
    ip.fromString(value);
  } else if ( key == String(FPSTR(cfg_dns_server)) ) {
    dns_ip.fromString(value);
  } else if ( key == String(FPSTR(cfg_subnet)) ) {
    subnet.fromString(value);
  } else if ( key == String(FPSTR(cfg_gateway)) ) {
    gateway.fromString(value);
  } else if ( key == String(FPSTR(cfg_ntp_server1)) ) {
    value.toCharArray(ntp_server1, 40);
  } else if ( key == String(FPSTR(cfg_ntp_server2)) ) {
    value.toCharArray(ntp_server2, 40);

  } else if ( key == String(FPSTR(cfg_mqtt_server)) ) {
    value.toCharArray(mqtt_server, 40);
  } else if ( key == String(FPSTR(cfg_mqtt_port)) ) {
    value.toCharArray(mqtt_port, 6);
  } else if ( key == String(FPSTR(cfg_mqtt_name)) ) {
    value.toCharArray(mqtt_name, 21);
  } else if ( key == String(FPSTR(cfg_mqtt_user)) ) {
    value.toCharArray(mqtt_user, 21);
  } else if ( key == String(FPSTR(cfg_mqtt_passwd)) ) {
    value.toCharArray(mqtt_passwd, 33);
  } else if ( key == String(FPSTR(cfg_mqtt_interval)) ) {
    mqtt_interval = value.toInt();
  } else if ( key == String(FPSTR(cfg_mqtt_watchdog)) ) {
    mqtt_watchdog = value.toInt();

  } else if ( key == String(FPSTR(cfg_host_name)) ) {
    value.toCharArray(host_name, 21);
  } else if ( key == String(FPSTR(cfg_syslog_server)) ) {
    value.toCharArray(syslog_server, 40);

  } else if (! sensorUpdateConfig(key, value)) {
    logString = String("UNKNOWN config parameter: " + key);
    mqttLog(logString);
    return;
  }
}

void runAction(String key, String value) {
  if ( key == "reboot" ) {
    logString = F("Received reboot command.");
    mqttLog(logString);
    mqttSend(String(FPSTR(mqttstr_online)), String(FPSTR(str_false)), true);
    ESP.restart();
  } else if ( key == "saveconfig" ) {
    saveConfig();
  } else if ( key == "savewifi" ) {
    WiFi.begin(ssid.c_str(), psk.c_str(), 0, NULL, false);
  } else if (! sensorRunAction(key, value)) {
    logString = String("UNKNOWN action: " + key);
    mqttLog(logString);
    return;
  }
}


void updateFirmware(String url) {
  logString = F("Firmware upgrade requested via MQTT: ");
  logString = logString + url;
  mqttLog(logString);

  t_httpUpdate_return ret = ESPhttpUpdate.update(url);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      logString = String(F("HTTP_UPDATE_FAILED Error (")) + ESPhttpUpdate.getLastError() + "): " + ESPhttpUpdate.getLastErrorString().c_str();
      mqttSend(String("$ota/error"), logString, false);
      break;

    case HTTP_UPDATE_NO_UPDATES:
      logString = F("HTTP_UPDATE_NO_UPDATES");
      mqttSend(String("$ota/error"), logString, false);
      break;

    case HTTP_UPDATE_OK:
      logString = F("HTTP_UPDATE_OK");
      mqttSend(String("$ota/command"), logString, false);
      break;
  }
  mqttLog(logString);
}


void setupSyslog() {
  // Setup syslog

  logString = String(F("Syslog server: ")) + String(syslog_server);
  logMessage(app_name_sys, logString, true);

  syslog.server(syslog_server, 514);
  syslog.deviceHostname(host_name);
  syslog.defaultPriority(LOG_KERN);
  // Send log messages up to LOG_INFO severity
  syslog.logMask(LOG_UPTO(LOG_INFO));
}

bool newWiFiCredentials() {
  connectNewWifi = false;
  bool result;

  logString = String(F("Request to change WiFi network from '")) + ssid + "' to '" + _ssid + String(F("'. Disconnecting from existing network."));
  logMessage(app_name_wifi, logString, true);

  delay(100);
  WiFi.persistent(true);  // Discard old WiFi credentials, i.e. erase the WiFi credentials
  WiFi.disconnect();      // Disconnect
  delay(100);

  // using user-provided  _ssid, _pass in place of system-stored ssid and pass
  WiFi.begin(_ssid.c_str(), _psk.c_str());

  logString = F("Waiting for connection result with 10s timeout");
  printMessage(app_name_wifi, logString, true);

  unsigned long start = millis();
  boolean keepConnecting = true;
  uint8_t wifiStatus;
  while (keepConnecting) {
    wifiStatus = WiFi.status();
    if (millis() > start + 10000) {
      keepConnecting = false;
      logString = F("Connection timed out");
      printMessage(app_name_wifi, logString, true);
    }
    if (wifiStatus == WL_CONNECTED || wifiStatus == WL_CONNECT_FAILED) {
      keepConnecting = false;
    }
    Serial.print('.');
    delay(500);
  }

  logString = String(F("Connection result: ")) + String(wifiStatus);
  logMessage(app_name_wifi, logString, true);

  if (wifiStatus != WL_CONNECTED) {
    logString    = F("Failed to connect.");
    result       = false;
    httpLoggedin = false;  // If this was done over a network connection record it as no-one logged in to allow the watchdog to reboot quicker.
  } else {
    logString = F("Connected.");
    WiFi.mode(WIFI_STA);
    ssid   = _ssid;
    psk    = _psk;
    result = true;
  }
  waitForDHCPLease();

  logMessage(app_name_wifi, logString, true);

  return result;
}

void checkWatchdog() {
  currentTime = millis();
  if ( currentTime >= (watchdog + (1000 * mqtt_watchdog ) ) && ! httpLoggedin) {
    logString = String(F("No MQTT data in ")) + String(mqtt_watchdog) + F(" seconds. ") + str_rebooting;
    logMessage(app_name_sys, logString, true);

    delay(1000);
    //reboot and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }
}

void waitForDHCPLease() {
  // Check if an IP address lease has been received and loop until it has
  IPAddress dhcpIP = WiFi.localIP();
  bool hasLease = (dhcpIP != INADDR_NONE);

  if (hasLease) {
    return;
  } else {
    logString = F("Waiting max 15 seconds for DHCP lease.");
    printMessage(app_name_net, logString, true);
    byte disconnect_count = 0;

    unsigned long start = millis();
    boolean keepConnecting = true;
    while (keepConnecting) {
      dhcpIP = WiFi.localIP();
      if (dhcpIP != INADDR_NONE) {
        keepConnecting = false;
      } else {
        if ( // We'll have a couple of goes at disconnecting and reconnecting as DHCP doesn't always start otherwise
          ( (millis() > start + 5000) && ( disconnect_count = 0 ) ) || 
          ( (millis() > start + 10000) && ( disconnect_count = 1 ) )
          ) {
          WiFi.persistent(false);  // KEEP old WiFi credentials, i.e. don't erase the WiFi credentials
          WiFi.disconnect();       // Disconnect
          delay(100);
          WiFi.begin();
          disconnect_count++;
        }

        if (millis() > start + 15000) {
          keepConnecting = false;
          Serial.println();
          logString = F("DHCP timed out");
          printMessage(app_name_net, logString, true);
        }
        Serial.print('.');
        delay(500);
      }
    }
    Serial.println();

    // WiFi.begin() may force the DHCP to get a new IP along with a new connection.
    // As such another go at a begin() might do the trick instead

    if (dhcpIP == INADDR_NONE) {
      logString = String(F("No DHCP lease in 15 seconds. ")) + str_rebooting;
      printMessage(app_name_net, logString, true);
  
      delay(1000);
      //reboot and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(5000);
    }
  }

}
