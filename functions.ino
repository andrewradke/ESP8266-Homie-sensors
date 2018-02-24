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
  mqttSend(String("$online"), String("true"), true);
  mqttSend(String("$fwname"), String(fwname), true);
  mqttSend(String("$fwversion"), String(FWVERSION), true);
  mqttSend(String("$name"), String(mqtt_name), true);
  mqttSend(String("$nodes"), String(nodes), true);
  mqttSend(String("$localip"), IPtoString(WiFi.localIP()), true);

  String cfgStr = String("$config/");

  mqttSend(String(cfgStr + "ssid"), String(ssid), true);

#ifdef STATICIP
  mqttSend(String(cfgStr + "static_ip"), String("true"), true);
  mqttSend(String(cfgStr + "ip_address"), IPtoString(ip), true);
  mqttSend(String(cfgStr + "subnet"), IPtoString(subnet), true);
  mqttSend(String(cfgStr + "gateway"), IPtoString(gateway), true);
  mqttSend(String(cfgStr + "dns_server"), IPtoString(dns_ip), true);
#else
  mqttSend(String(cfgStr + "static_ip"), String("false"), true);
#endif

  mqttSend(String(cfgStr + "mqtt_server"), String(mqtt_server), true);
  mqttSend(String(cfgStr + "mqtt_port"), String(mqtt_port), true);
  mqttSend(String(cfgStr + "mqtt_name"), String(mqtt_name), true);
  mqttSend(String(cfgStr + "mqtt_interval"), String(mqtt_interval), true);

  mqttSend(String(cfgStr + "use_syslog"), String(use_syslog), true);
  mqttSend(String(cfgStr + "host_name"), String(host_name), true);
  mqttSend(String(cfgStr + "syslog_server"), String(syslog_server), true);

// #################### printSensorConfig() is defined in each sensor file as appropriate
  printSensorConfig(cfgStr);
}


void printSystem() {
  String sysStr = String("$system/");
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
  Serial.println("Should save config");
  shouldSaveConfig = true;
}


void saveConfig() {
  app_name = "CFG";
  syslog.appName(app_name);

  logString = "Saving config";
  mqttLog(logString);

  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();

  json["use_staticip"]  = use_staticip;

  JsonArray& json_ip = json.createNestedArray("ip_address");
  json_ip.add(ip[0]);
  json_ip.add(ip[1]);
  json_ip.add(ip[2]);
  json_ip.add(ip[3]);
  JsonArray& json_subnet = json.createNestedArray("subnet");
  json_subnet.add(subnet[0]);
  json_subnet.add(subnet[1]);
  json_subnet.add(subnet[2]);
  json_subnet.add(subnet[3]);
  JsonArray& json_gateway = json.createNestedArray("gateway");
  json_gateway.add(gateway[0]);
  json_gateway.add(gateway[1]);
  json_gateway.add(gateway[2]);
  json_gateway.add(gateway[3]);
  JsonArray& json_dns = json.createNestedArray("dns_server");
  json_dns.add(dns_ip[0]);
  json_dns.add(dns_ip[1]);
  json_dns.add(dns_ip[2]);
  json_dns.add(dns_ip[3]);

  json["mqtt_server"]   = mqtt_server;
  json["mqtt_port"]     = mqtt_port;
  json["mqtt_name"]     = mqtt_name;
  json["mqtt_tls"]      = mqtt_tls;
  json["mqtt_auth"]     = mqtt_auth;
  json["mqtt_user"]     = mqtt_user;
  json["mqtt_passwd"]   = mqtt_passwd;
  json["mqtt_interval"] = mqtt_interval;

  json["use_syslog"]    = use_syslog;
  json["host_name"]     = host_name;
  json["syslog_server"] = syslog_server;

  sensorExportJSON(json);

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    logString = "Failed to open config file for writing.";
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
  app_name = "CFG";
  syslog.appName(app_name);

  //read configuration from FS json
  logString = "Mounting FS...";
  printMessage(logString, true);

  if (SPIFFS.begin()) {
    if (SPIFFS.exists("/config.json")) {
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        logString = "Opened config file.";
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
          if (json["use_staticip"].is<bool>()) {
            use_staticip = json["use_staticip"];
          }
          if (json["ip_address"].is<JsonArray&>()) {
            ip[0]        = json["ip_address"][0];
            ip[1]        = json["ip_address"][1];
            ip[2]        = json["ip_address"][2];
            ip[3]        = json["ip_address"][3];
          }
          if (json["subnet"].is<JsonArray&>()) {
            subnet[0]    = json["subnet"][0];
            subnet[1]    = json["subnet"][1];
            subnet[2]    = json["subnet"][2];
            subnet[3]    = json["subnet"][3];
          }
          if (json["gateway"].is<JsonArray&>()) {
            gateway[0]   = json["gateway"][0];
            gateway[1]   = json["gateway"][1];
            gateway[2]   = json["gateway"][2];
            gateway[3]   = json["gateway"][3];
          }
          if (json["dns_server"].is<JsonArray&>()) {
            dns_ip[0]    = json["dns_server"][0];
            dns_ip[1]    = json["dns_server"][1];
            dns_ip[2]    = json["dns_server"][2];
            dns_ip[3]    = json["dns_server"][3];
          }


          if (json["mqtt_server"].is<const char*>()) {
            strcpy(mqtt_server,   json["mqtt_server"]);
          }
          if (json["mqtt_port"].is<const char*>()) {
            strcpy(mqtt_port,     json["mqtt_port"]);
          }
          if (json["mqtt_name"].is<const char*>()) {
            strcpy(mqtt_name,     json["mqtt_name"]);
          }
          if (json["mqtt_tls"].is<bool>()) {
            mqtt_tls = json["mqtt_tls"];
          }
          if (json["mqtt_auth"].is<bool>()) {
            mqtt_auth = json["mqtt_auth"];
          }
          if (json["mqtt_user"].is<const char*>()) {
            strcpy(mqtt_user,     json["mqtt_user"]);
          }
          if (json["mqtt_passwd"].is<const char*>()) {
            strcpy(mqtt_passwd,   json["mqtt_passwd"]);
          }
          if (json["mqtt_interval"].is<int>()) {
            mqtt_interval = json["mqtt_interval"];
          }


          if (json["use_syslog"].is<bool>()) {
            use_syslog = json["use_syslog"];
          }
          if (json["host_name"].is<const char*>()) {
            strcpy(host_name,     json["host_name"]);
          }
          if (json["syslog_server"].is<const char*>()) {
            strcpy(syslog_server, json["syslog_server"]);
          }

          sensorImportJSON(json);
        } else {
          logString = "Corrupted json config file.";
          printMessage(logString, true);
        }
      } else {
        logString = "Failed to open config file.";
        printMessage(logString, true);
      }
    } else {
      logString = "No existing config file.";
      printMessage(logString, true);
    }
  } else {
    logString = "Failed to mount FS.";
    printMessage(logString, true);
  }
}  //end readConfig


void updateConfig(String key, String value) {
  app_name = "CFG";
  syslog.appName(app_name);

  if ( key == "ssid" ) {
    ssid = value;
  } else if ( key == "psk" ) {
    psk = value;

#ifdef STATICIP
  } else if ( key == "ip_address" ) {
    ip.fromString(value);
  } else if ( key == "dns_server" ) {
    dns_ip.fromString(value);
  } else if ( key == "subnet" ) {
    subnet.fromString(value);
  } else if ( key == "gateway" ) {
    gateway.fromString(value);
#endif

  } else if ( key == "mqtt_server" ) {
    value.toCharArray(mqtt_server, 40);
  } else if ( key == "mqtt_port" ) {
    value.toCharArray(mqtt_port, 6);
  } else if ( key == "mqtt_name" ) {
    value.toCharArray(mqtt_name, 21);
  } else if ( key == "mqtt_user" ) {
    value.toCharArray(mqtt_user, 21);
  } else if ( key == "mqtt_passwd" ) {
    value.toCharArray(mqtt_passwd, 33);

  } else if ( key == "host_name" ) {
    value.toCharArray(host_name, 21);
  } else if ( key == "syslog_server" ) {
    value.toCharArray(syslog_server, 40);

  } else if (! sensorUpdateConfig(key, value)) {
    logString = String("UNKNOWN config parameter: " + key);
    app_name = "CFG";
    mqttLog(logString);
    return;
  }
}

void runAction(String key, String value) {
  app_name = "SYS";
  syslog.appName(app_name);

  if ( key == "reboot" ) {
    logString = "Received reboot command.";
    mqttLog(logString);
    mqttSend(String("$online"), String("false"), true);
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
  app_name = "SYS";
  syslog.appName(app_name);

  logString = String("Firmware upgrade requested: ");
  logString = logString + url;
  mqttLog(logString);

  t_httpUpdate_return ret = ESPhttpUpdate.update(url);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      logString = String("HTTP_UPDATE_FAILED Error (");
      logString = String(logString + ESPhttpUpdate.getLastError() );
      logString = String(logString + "): " + ESPhttpUpdate.getLastErrorString().c_str());
      mqttSend(String("$ota/error"), logString, false);
      break;

    case HTTP_UPDATE_NO_UPDATES:
      logString = String("HTTP_UPDATE_NO_UPDATES");
      mqttSend(String("$ota/error"), logString, false);
      break;

    case HTTP_UPDATE_OK:
      logString = String("HTTP_UPDATE_OK");
      mqttSend(String("$ota/command"), logString, false);
      break;
  }
  mqttLog(logString);
}


void setupSyslog() {
    // Setup syslog
    logString = "Syslog server: " + String(syslog_server);
    printMessage(logString, true);
  
    syslog.server(syslog_server, 514);
    syslog.deviceHostname(host_name);
    syslog.defaultPriority(LOG_KERN);
    // Send log messages up to LOG_INFO severity
    syslog.logMask(LOG_UPTO(LOG_INFO));
}
