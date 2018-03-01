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


void printMessage(String message, bool oled) {
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


void printConfig() {
  mqttSend(String(FPSTR(mqttstr_online)),       String("true"), true);
  mqttSend(String(FPSTR(mqttstr_fwname)),       String(fwname), true);
  mqttSend(String(FPSTR(mqttstr_fwversion)),    String(FWVERSION), true);
  mqttSend(String(FPSTR(mqttstr_name)),         String(mqtt_name), true);
  mqttSend(String(FPSTR(mqttstr_nodes)),        String(nodes), true);
  mqttSend(String(FPSTR(mqttstr_localip)),      IPtoString(WiFi.localIP()), true);

  String cfgStr = String(FPSTR(mqttstr_config));

  mqttSend(String(cfgStr + "ssid"),          String(ssid), true);

  if (use_staticip) {
    mqttSend(String(cfgStr + "static_ip"),   String("true"), true);
    mqttSend(String(cfgStr + String(FPSTR(cfg_ip_address))),  IPtoString(ip), true);
    mqttSend(String(cfgStr + String(FPSTR(cfg_subnet))),      IPtoString(subnet), true);
    mqttSend(String(cfgStr + String(FPSTR(cfg_gateway))),     IPtoString(gateway), true);
    mqttSend(String(cfgStr + String(FPSTR(cfg_dns_server))),  IPtoString(dns_ip), true);
  } else {
    mqttSend(String(cfgStr + "static_ip"),   String("false"), true);
    mqttSend(String(cfgStr + String(FPSTR(cfg_ip_address))),  "", true);
    mqttSend(String(cfgStr + String(FPSTR(cfg_subnet))),      "", true);
    mqttSend(String(cfgStr + String(FPSTR(cfg_gateway))),     "", true);
    mqttSend(String(cfgStr + String(FPSTR(cfg_dns_server))),  "", true);
  }

  mqttSend(String(cfgStr + String(FPSTR(cfg_ntp_server1))),   String(ntp_server1), true);
  mqttSend(String(cfgStr + String(FPSTR(cfg_ntp_server2))),   String(ntp_server2), true);

  mqttSend(String(cfgStr + String(FPSTR(cfg_mqtt_server))),   String(mqtt_server), true);
  mqttSend(String(cfgStr + String(FPSTR(cfg_mqtt_port))),     String(mqtt_port), true);
  mqttSend(String(cfgStr + String(FPSTR(cfg_mqtt_name))),     String(mqtt_name), true);
  mqttSend(String(cfgStr + String(FPSTR(cfg_mqtt_interval))), String(mqtt_interval), true);
  mqttSend(String(cfgStr + String(FPSTR(cfg_mqtt_watchdog))), String(mqtt_watchdog), true);

  mqttSend(String(cfgStr + String(FPSTR(cfg_use_syslog))),    String(use_syslog), true);
  mqttSend(String(cfgStr + String(FPSTR(cfg_host_name))),     String(host_name), true);
  mqttSend(String(cfgStr + String(FPSTR(cfg_syslog_server))), String(syslog_server), true);

// #################### printSensorConfig() is defined in each sensor file as appropriate
  printSensorConfig(cfgStr);
}


void printSystem() {
  String sysStr = String(FPSTR(mqttstr_system));
  mqttSend(String(sysStr + "flash_size"),        String(ESP.getFlashChipRealSize()/1048576.0) + " MB", true);
  mqttSend(String(sysStr + "flash_size_config"), String(ESP.getFlashChipSize()/1048576.0) + " MB",     true);
  mqttSend(String(sysStr + "program_size"),      String(ESP.getSketchSize() / 1024) + " kB",           true);
  mqttSend(String(sysStr + "free_program_size"), String(ESP.getFreeSketchSpace() / 1024) + " kB",      true);
  mqttSend(String(sysStr + "free_memory"),       String(ESP.getFreeHeap() / 1024) + " kB",             true);
  mqttSend(String(sysStr + "esp_chip_id"),       String(ESP.getChipId()),                              true);
  mqttSend(String(sysStr + "flash_chip_id"),     String(ESP.getFlashChipId()),                         true);
}


//callback notifying us of the need to save config
void saveConfigCallback () {
//  Serial.println("Should save config");
  shouldSaveConfig = true;
}


void saveConfig() {
  strncpy_P (app_name, app_name_cfg, sizeof(app_name_cfg) );
  syslog.appName(app_name);
  char* cfg_name;

  logString = "Saving config";
  mqttLog(logString);

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

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    logString = String(F("Failed to open config file for writing."));
    printMessage(logString, true);
    if (use_syslog) {
      syslog.log(LOG_ERR, logString);
    }
  }

  dmesg();
  json.printTo(Serial);
  Serial.println();
  json.printTo(configFile);
  configFile.close();
}  //end saveConfig


void loadConfig() {
  strncpy_P (app_name, app_name_cfg, sizeof(app_name_cfg) );
  syslog.appName(app_name);

  //read configuration from FS json
  logString = String(F("Mounting FS..."));
  printMessage(logString, true);

  if (SPIFFS.begin()) {
    if (SPIFFS.exists("/config.json")) {
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        logString = String(F("Opened config file."));
        printMessage(logString, true);

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

          sensorImportJSON(json);
        } else {
          logString = String(F("Corrupted json config file."));
          printMessage(logString, true);
        }
      } else {
        logString = String(F("Failed to open config file."));
        printMessage(logString, true);
      }
    } else {
      logString = String(F("No existing config file."));
      printMessage(logString, true);
    }
  } else {
    logString = String(F("Failed to mount FS."));
    printMessage(logString, true);
  }
}  //end readConfig


void updateConfig(String key, String value) {
  strncpy_P (app_name, app_name_cfg, sizeof(app_name_cfg) );
  syslog.appName(app_name);

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
    strncpy_P (app_name, app_name_cfg, sizeof(app_name_cfg) );
    mqttLog(logString);
    return;
  }
}

void runAction(String key, String value) {
  strncpy_P (app_name, app_name_sys, sizeof(app_name_sys) );
  syslog.appName(app_name);

  if ( key == "reboot" ) {
    logString = String(F("Received reboot command."));
    mqttLog(logString);
    mqttSend(String(FPSTR(mqttstr_online)), String("false"), true);
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
  strncpy_P (app_name, app_name_sys, sizeof(app_name_sys) );
  syslog.appName(app_name);

  logString = String(F("Firmware upgrade requested: "));
  logString = logString + url;
  mqttLog(logString);

  t_httpUpdate_return ret = ESPhttpUpdate.update(url);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      logString = String(F("HTTP_UPDATE_FAILED Error ("));
      logString = String(logString + ESPhttpUpdate.getLastError() );
      logString = String(logString + "): " + ESPhttpUpdate.getLastErrorString().c_str());
      mqttSend(String("$ota/error"), logString, false);
      break;

    case HTTP_UPDATE_NO_UPDATES:
      logString = String(F("HTTP_UPDATE_NO_UPDATES"));
      mqttSend(String("$ota/error"), logString, false);
      break;

    case HTTP_UPDATE_OK:
      logString = String(F("HTTP_UPDATE_OK"));
      mqttSend(String("$ota/command"), logString, false);
      break;
  }
  mqttLog(logString);
}


void setupSyslog() {
    // Setup syslog
    logString = String(F("Syslog server: ")) + String(syslog_server);
    printMessage(logString, true);
  
    syslog.server(syslog_server, 514);
    syslog.deviceHostname(host_name);
    syslog.defaultPriority(LOG_KERN);
    // Send log messages up to LOG_INFO severity
    syslog.logMask(LOG_UPTO(LOG_INFO));
}
