void httpSetup() {

  httpServer.onNotFound([]() {
    httpData = httpHeader;
    httpData += "404: not found";
    httpData += httpFooter;
    httpServer.send(404, "text/html", httpData);
  });

  
  httpServer.on("/", []() {
    if(! httpServer.authenticate(www_username, www_password))
      return httpServer.requestAuthentication();
    httpData = httpHeader;
    if (rebootRequired) {
      httpData += "<p><b>Reboot required to use new settings.</b></p>";
    }

    httpData += httpSensorData();
    httpData += "<br/>";

    /// send uptime and signal
    unsigned long uptime = millis();
    uptime = uptime / 1000;
    int signal = WiFi.RSSI();
    httpData += tableStart;
    httpData += trStart + "Uptime:" + tdBreak + String(uptime) + " s" + trEnd;
    httpData += trStart + "Signal:" + tdBreak + String(signal) + " dBm" + trEnd;
    httpData += trEnd;
    httpData += tableEnd;

    httpData += "<meta http-equiv='refresh' content='3' />";

    httpData += httpFooter;
    httpServer.send(200, "text/html", httpData);
  });

  httpServer.on("/reboot", []() {
    if(! httpServer.authenticate(www_username, www_password))
      return httpServer.requestAuthentication();
    httpData = "<html><head><meta http-equiv='refresh' content='3;url=/' /></head><p>Rebooting...</p>";
    httpData += httpFooter;
    httpServer.send(200, "text/html", httpData);
    delay(100);
    ESP.restart();
    delay(5000);
  });

  httpServer.on("/system", [](){
    if(! httpServer.authenticate(www_username, www_password))
      return httpServer.requestAuthentication();
    httpData = httpHeader;
    if (rebootRequired) {
      httpData += "<p><b>Reboot required to use new settings.</b></p>";
    }

    httpData += tableStart;

    httpData += trStart + "Flash size:" + tdBreak;
    httpData += String(ESP.getFlashChipRealSize()/1048576.0) + " MB";
    httpData += trEnd;

    httpData += trStart + "Flash size config:" + tdBreak;
    httpData += String(ESP.getFlashChipSize()/1048576.0) + " MB";
    httpData += trEnd;

    httpData += trStart + "Program size:" + tdBreak;
    httpData += String(ESP.getSketchSize() / 1024) + " kB";
    httpData += trEnd;

    httpData += trStart + "Free program size:" + tdBreak;
    httpData += String(ESP.getFreeSketchSpace() / 1024) + " kB";
    httpData += trEnd;

    httpData += trStart + "Free memory:" + tdBreak;
    httpData += String(ESP.getFreeHeap() / 1024) + " kB";
    httpData += trEnd;

    httpData += trStart + "ESP chip ID:" + tdBreak;
    httpData += String(ESP.getChipId());
    httpData += trEnd;

    httpData += trStart + "Flash chip ID:" + tdBreak;
    httpData += String(ESP.getFlashChipId());
    httpData += trEnd;


    /// send uptime and signal
    unsigned long uptime = millis();
    uptime = uptime / 1000;
    int signal = WiFi.RSSI();
    httpData += trStart + "Uptime:" + tdBreak + String(uptime) + " s" + trEnd;
    httpData += trStart + "Signal:" + tdBreak + String(signal) + " dBm" + trEnd;
    httpData += trEnd;
    httpData += tableEnd;

    httpData += "<meta http-equiv='refresh' content='3' />";
    httpData += httpFooter;
    httpServer.send(200, "text/html", httpData);
  });


  httpServer.on("/config", [](){
    if(! httpServer.authenticate(www_username, www_password))
      return httpServer.requestAuthentication();
    httpData = httpHeader;

    if (httpServer.args()) {
      IPAddress tmpip;

      if (httpServer.hasArg("staticip")) {
        if (httpServer.arg("staticip") == "true") {
          if (! use_staticip) {
            rebootRequired = true;      // Changing to a static IP requires a reboot
            use_staticip = true;
          }
        } else {
          if (use_staticip) {
            rebootRequired = true;      // Changing to a static IP requires a reboot
            use_staticip = false;
          }
        }
      }

      if (httpServer.hasArg("ip_address")) {
        tmpip.fromString(httpServer.arg("ip_address"));
        if (ip != tmpip) {
          if (use_staticip)
            rebootRequired = true;      // Changing IP requires a reboot
          ip = tmpip;
        }
      }
      if (httpServer.hasArg("subnet")) {
        tmpip.fromString(httpServer.arg("subnet"));
        if (subnet != tmpip) {
          if (use_staticip)
            rebootRequired = true;      // Changing subnet requires a reboot
          subnet = tmpip;
        }
      }
      if (httpServer.hasArg("gateway")) {
        tmpip.fromString(httpServer.arg("gateway"));
        if (gateway != tmpip) {
          if (use_staticip)
            rebootRequired = true;      // Changing gateway IP requires a reboot
          gateway = tmpip;
        }
      }
      if (httpServer.hasArg("dns_ip")) {
        tmpip.fromString(httpServer.arg("dns_ip"));
        if (dns_ip != tmpip) {
          if (use_staticip)
            rebootRequired = true;      // Changing DNS server requires a reboot
          dns_ip = tmpip;
        }
      }

      if (httpServer.hasArg("mqtt_server")) {
        tmpString = String(mqtt_server);
        if (httpServer.arg("mqtt_server") != tmpString) {
          mqttClient.disconnect();        // MQTT will reconnect automatically with the new value
          httpServer.arg("mqtt_server").toCharArray(mqtt_server, 40);
        }
      }
      if (httpServer.hasArg("mqtt_port")) {
        tmpString = String(mqtt_port);
        if (httpServer.arg("mqtt_port") != tmpString) {
          mqttClient.disconnect();        // MQTT will reconnect automatically with the new value
          httpServer.arg("mqtt_port").toCharArray(mqtt_port, 5);
        }
      }
      if (httpServer.hasArg("mqtt_name")) {
        tmpString = String(mqtt_name);
        if (httpServer.arg("mqtt_name") != tmpString) {
          mqttClient.disconnect();        // MQTT will reconnect automatically with the new value
          httpServer.arg("mqtt_name").toCharArray(mqtt_name, 20);
        }
      }
      if (httpServer.hasArg("mqtt_tls")) {
        if (httpServer.arg("mqtt_tls") == "on") {
          if (!mqtt_tls) {
            mqttClient.disconnect();        // MQTT will reconnect automatically with the new value
            mqtt_tls = true;
          }
        } else {
          if (mqtt_tls) {
            mqttClient.disconnect();        // MQTT will reconnect automatically with the new value
            mqtt_tls = false;
          }
        }
      } else {
        if (mqtt_tls) {
          mqttClient.disconnect();        // MQTT will reconnect automatically with the new value
          mqtt_tls = false;
        }
      }
      if (httpServer.hasArg("mqtt_auth")) {
        if (httpServer.arg("mqtt_auth") == "on") {
          if (!mqtt_auth) {
            mqttClient.disconnect();        // MQTT will reconnect automatically with the new value
            mqtt_auth = true;
          }
        } else {
          if (mqtt_auth) {
            mqttClient.disconnect();        // MQTT will reconnect automatically with the new value
            mqtt_auth = false;
          }
        }
      } else {
        if (mqtt_auth) {
          mqttClient.disconnect();        // MQTT will reconnect automatically with the new value
          mqtt_auth = false;
        }
      }
      if (httpServer.hasArg("mqtt_user")) {
        tmpString = String(mqtt_user);
        if (httpServer.arg("mqtt_user") != tmpString) {
          mqttClient.disconnect();        // MQTT will reconnect automatically with the new value
          httpServer.arg("mqtt_user").toCharArray(mqtt_user, 20);
        }
      }
      if (httpServer.hasArg("mqtt_passwd")) {
        tmpString = String(mqtt_passwd);
        if (httpServer.arg("mqtt_passwd") != tmpString) {
          mqttClient.disconnect();        // MQTT will reconnect automatically with the new value
          httpServer.arg("mqtt_passwd").toCharArray(mqtt_passwd, 32);
        }
      }

      if (httpServer.hasArg("mqtt_interval"))
        mqtt_interval = httpServer.arg("mqtt_interval").toInt();

      if (httpServer.hasArg("syslog_server"))
        httpServer.arg("syslog_server").toCharArray(syslog_server, 40);
      if (httpServer.hasArg("use_syslog")) {
        if (httpServer.arg("use_syslog") == "on") {
          if (! use_syslog)
            setupSyslog();      // Syslog changed to enabled
          use_syslog = true;
        } else {
          use_syslog = false;
        }
      } else {
        use_syslog = false;
      }

      saveConfig();
    }

    httpData += "<form action='#' method='POST'>";
    httpData += tableStart;

    if (rebootRequired) {
      httpData += "<p><b>Reboot required to use new settings.</b></p>";
    }

    httpData += trStart + "<b>Network:</b>" + tdBreak + trEnd;
    httpData += trStart + "IP:"                 + tdBreak;
    httpData += htmlRadio("staticip", "true",          use_staticip)     + "Static IP";
    httpData += htmlRadio("staticip", "false",         (! use_staticip)) + "DHCP";
    httpData += trEnd;
    httpData += trStart + "Address:"            + tdBreak + htmlInput("text",     "ip_address",    IPtoString(ip))      + trEnd;
    httpData += trStart + "Subnet Mask:"        + tdBreak + htmlInput("text",     "subnet",        IPtoString(subnet))  + trEnd;
    httpData += trStart + "Gateway:"            + tdBreak + htmlInput("text",     "gateway",       IPtoString(gateway)) + trEnd;
    httpData += trStart + "DNS Server:"         + tdBreak + htmlInput("text",     "dns_ip",        IPtoString(dns_ip))  + trEnd;

    httpData += trStart + "<b>MQTT:</b>" + tdBreak + trEnd;

    httpData += trStart + "Server:"             + tdBreak + htmlInput("text",     "mqtt_server",   mqtt_server)   + trEnd;
    httpData += trStart + "Port:"               + tdBreak + htmlInput("text",     "mqtt_port",     mqtt_port)     + trEnd;
    httpData += trStart + "Name:"               + tdBreak + htmlInput("text",     "mqtt_name",     mqtt_name)     + trEnd;
    httpData += trStart + "Use TLS:"            + tdBreak + htmlCheckbox(         "mqtt_tls",      mqtt_tls)      + trEnd;
    httpData += trStart + "Use authentication:" + tdBreak + htmlCheckbox(         "mqtt_auth",     mqtt_auth)     + trEnd;
    httpData += trStart + "Username:"           + tdBreak + htmlInput("text",     "mqtt_user",     mqtt_user)     + trEnd;
    httpData += trStart + "Password:"           + tdBreak + htmlInput("text",     "mqtt_passwd",   mqtt_passwd)   + trEnd;
    httpData += trStart + "Data interval (s):"  + tdBreak + htmlInput("text",     "mqtt_interval", String(mqtt_interval)) + trEnd;

    httpData += trStart + "<b>Syslog:</b>" + tdBreak + trEnd;

    httpData += trStart + "Use Syslog:"         + tdBreak + htmlCheckbox(         "use_syslog",    use_syslog)    + trEnd;
    httpData += trStart + "Server:"             + tdBreak + htmlInput("text",     "syslog_server", syslog_server) + trEnd;

    httpData += trStart + "<b>Sensors:</b>" + tdBreak + trEnd;

/*
// #################### any extra WM config #defined in variables.h
#ifdef WMADDCONFIG
WMADDCONFIG
#endif
*/
    httpData += trEnd;
    httpData += tableEnd;

    httpData += "<input type='submit' value='Submit'><input type='Reset'></form><br/>";

    httpData += httpFooter;
    httpServer.send(200, "text/html", httpData);
  });
}

String htmlInput(String inputType, String inputName, String inputValue) {
  return String("<input type='" + inputType + "' name='" + inputName + "' value='" + inputValue + "'>");
}
String htmlCheckbox(String inputName, const bool inputChecked) {
  String tmpString = "<input type='checkbox' name='" + inputName + "'";
  if (inputChecked) {
    tmpString += " checked";
  }
  tmpString += ">";
  return tmpString;
}
String htmlRadio(String inputName, String inputValue, const bool inputChecked) {
  String tmpString = "<input type='radio' name='" + inputName + "' value='" + inputValue + "'";
  if (inputChecked) {
    tmpString += " checked";
  }
  tmpString += ">";
  return tmpString;
}

