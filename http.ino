String anchars = "abcdefghijklmnopqrstuvwxyz0123456789";                     // Alfanumberic character filled string for cookie generation
String sessioncookie;                                                        // This is cookie buffer

void httpSetup() {
  gencookie(); //generate new cookie on device start

  httpServer.onNotFound([]() {
    httpData = htmlHeader("");
    httpData += "404: not found";
    httpData += httpFooter;
    httpServer.send(404, "text/html", httpData);
  });

  httpServer.on("/login", []() {
    String loginPage = "<!DOCTYPE html><html><head><title>Login</title></head><body> <div id=\"login\"> <form action='/login' method='POST'> <center> <h1>Login </h1><p><input type='text' name='user' placeholder='User name'></p><p><input type='password' name='pass' placeholder='Password'></p><br><button type='submit' name='submit'>login</button></center> </form></body></html>";

    if (httpServer.hasHeader("Cookie")){   
      String cookie = httpServer.header("Cookie"); // Copy the Cookie header to this buffer
    }

    // if user posted with these arguments
    if (httpServer.hasArg("user") && httpServer.hasArg("pass")) {
      // check if login details are good and dont allow it if device is in lockdown
      if (httpServer.arg("user") == httpUser &&  httpServer.arg("pass") == httpPasswd && !lock) {
        // if above values are good, send 'Cookie' header with variable c, with format 'c=sessioncookie'
        httpServer.sendHeader("Location","/");
        httpServer.sendHeader("Cache-Control","no-cache");
        httpServer.sendHeader("Set-Cookie","c=" + sessioncookie);
        httpServer.send(301);
        trycount = 0;                                 // With good headers in mind, reset the trycount buffer
        return;
      }

      loginPage += "<center><br>";
      if (trycount != 10 && !lock)
        trycount++;                                   // If system is not locked up the trycount buffer
      if (trycount < 10 && !lock) {                   // We go here if systems isn't locked out, we give user 10 times to make a mistake after we lock down the system, thus making brute force attack almost imposible
        loginPage += "<p>Wrong username/password</p>";
        loginPage += "You have ";
        loginPage += (10 - trycount);
        loginPage += " tries before system temporarily locks out.";
        logincld = millis();                          // Reset the logincld timer, since we still have available tries
      }
      
      if (trycount == 10) {                           // If too many bad tries
        loginPage += "Too many invalid login requests, you can try again in ";
        if (lock) {
          loginPage += 5 - ((millis() - logincld) / 60000); // Display lock time remaining in minutes
        } else {
          logincld = millis();
          lock = true;
          loginPage += "5";                                 // This happens when your device first locks down
        }
        loginPage += " minutes.";
      }
    }
    loginPage += "</center>";
    httpServer.send(200, "text/html", loginPage );
  });

  httpServer.on("/logout", []() {
    //Set 'c=0', it users header, effectively deleting it's header 
    httpServer.sendHeader("Set-Cookie","c=0");
    httpServer.sendHeader("Location","/login");
    httpServer.sendHeader("Cache-Control","no-cache");
    httpServer.send(301);
    return;
  });

  httpServer.on("/", []() {
    if (!is_authenticated()) {
      httpServer.sendHeader("Location","/login");
      httpServer.sendHeader("Cache-Control","no-cache");
      httpServer.send(301);
      return;
    }
    tempign = millis(); //reset the inactivity timer if someone logs in

    httpData = htmlHeader("/");

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
    if (!is_authenticated()) {
      httpServer.sendHeader("Location","/login");
      httpServer.sendHeader("Cache-Control","no-cache");
      httpServer.send(301);
      return;
    }
    tempign = millis(); //reset the inactivity timer if someone logs in

    httpData = "<html><head><meta http-equiv='refresh' content='3;url=/' /></head><p>Rebooting...</p>";
    httpData += httpFooter;
    httpServer.send(200, "text/html", httpData);
    delay(100);
    ESP.restart();
    delay(5000);
  });

  httpServer.on("/system", [](){
    if (!is_authenticated()) {
      httpServer.sendHeader("Location","/login");
      httpServer.sendHeader("Cache-Control","no-cache");
      httpServer.send(301);
      return;
    }
    tempign = millis(); //reset the inactivity timer if someone logs in

    httpData = htmlHeader("/system");

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
    if (!is_authenticated()) {
      httpServer.sendHeader("Location","/login");
      httpServer.sendHeader("Cache-Control","no-cache");
      httpServer.send(301);
      return;
    }
    tempign = millis(); //reset the inactivity timer if someone logs in

    httpData = htmlHeader("/config");

    if (httpServer.args()) {
      IPAddress tmpip;

      if (httpServer.hasArg("staticip")) {
        if (httpServer.arg("staticip") == "true") {
          if (! use_staticip) {
            rebootRequired = true;        // Changing to a static IP requires a reboot
            use_staticip = true;
          }
        } else {
          if (use_staticip) {
            rebootRequired = true;        // Changing to a static IP requires a reboot
            use_staticip = false;
          }
        }
      }

      if (httpServer.hasArg("ip_address")) {
        tmpip.fromString(httpServer.arg("ip_address"));
        if (ip != tmpip) {
          if (use_staticip)
            rebootRequired = true;        // Changing IP requires a reboot
          ip = tmpip;
        }
      }
      if (httpServer.hasArg("subnet")) {
        tmpip.fromString(httpServer.arg("subnet"));
        if (subnet != tmpip) {
          if (use_staticip)
            rebootRequired = true;        // Changing subnet requires a reboot
          subnet = tmpip;
        }
      }
      if (httpServer.hasArg("gateway")) {
        tmpip.fromString(httpServer.arg("gateway"));
        if (gateway != tmpip) {
          if (use_staticip)
            rebootRequired = true;        // Changing gateway IP requires a reboot
          gateway = tmpip;
        }
      }
      if (httpServer.hasArg("dns_ip")) {
        tmpip.fromString(httpServer.arg("dns_ip"));
        if (dns_ip != tmpip) {
          if (use_staticip)
            rebootRequired = true;        // Changing DNS server requires a reboot
          dns_ip = tmpip;
        }
      }
      if (httpServer.hasArg("ntp_server1")) {
        tmpString = String(ntp_server1);
        if (httpServer.arg("ntp_server1") != tmpString) {
          rebootRequired = true;          // Changing NTP servers requires a reboot
          httpServer.arg("ntp_server1").toCharArray(ntp_server1, 40);
        }
      }
      if (httpServer.hasArg("ntp_server2")) {
        tmpString = String(ntp_server2);
        if (httpServer.arg("ntp_server2") != tmpString) {
          rebootRequired = true;          // Changing NTP servers requires a reboot
          httpServer.arg("ntp_server2").toCharArray(ntp_server2, 40);
        }
      }

      if (httpServer.hasArg("mqtt_server")) {
        tmpString = String(mqtt_server);
        if (httpServer.arg("mqtt_server") != tmpString) {
          httpServer.arg("mqtt_server").toCharArray(mqtt_server, 40);
          mqttClient.disconnect();        // MQTT will reconnect automatically with the new value
          mqttClient.setServer(mqtt_server, atoi( mqtt_port ));
        }
      }
      if (httpServer.hasArg("mqtt_port")) {
        tmpString = String(mqtt_port);
        if (httpServer.arg("mqtt_port") != tmpString) {
          httpServer.arg("mqtt_port").toCharArray(mqtt_port, 5);
          mqttClient.disconnect();        // MQTT will reconnect automatically with the new value
          mqttClient.setServer(mqtt_server, atoi( mqtt_port ));
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
            rebootRequired = true;        // Enabling TLS requires a reboot, as the server certificate verification will fail and force a reboot otherwise
            mqtt_tls = true;
          }
        } else {
          if (mqtt_tls) {
            mqttClient.disconnect();      // MQTT will reconnect automatically with the new value
            mqttClient.setClient(espClient);
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
            mqttClient.disconnect();      // MQTT will reconnect automatically with the new value
            mqtt_auth = true;
          }
        } else {
          if (mqtt_auth) {
            mqttClient.disconnect();      // MQTT will reconnect automatically with the new value
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

      if (httpServer.hasArg("mqtt_watchdog")) {
        mqtt_watchdog = httpServer.arg("mqtt_watchdog").toInt();
        if (mqtt_watchdog <= mqtt_interval) {
          mqtt_watchdog = mqtt_interval + 1;
        }
        if (mqtt_watchdog < 60) {
          mqtt_watchdog = 60;
        }
      }

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

      httpSensorConfig();

      saveConfig();
    }

    httpData += "<form action='#' method='POST'>";
    httpData += tableStart;

    httpData += thStart + "<b>Network:</b>" + thBreak + thEnd;
    httpData += trStart + "IP:"                 + tdBreak;
    httpData += htmlRadio("staticip", "true",          use_staticip)     + "Static IP";
    httpData += htmlRadio("staticip", "false",         (! use_staticip)) + "DHCP";
    httpData += trEnd;
    httpData += trStart + "Address:"            + tdBreak + htmlInput("text",     "ip_address",    IPtoString(ip))      + trEnd;
    httpData += trStart + "Subnet Mask:"        + tdBreak + htmlInput("text",     "subnet",        IPtoString(subnet))  + trEnd;
    httpData += trStart + "Gateway:"            + tdBreak + htmlInput("text",     "gateway",       IPtoString(gateway)) + trEnd;
    httpData += trStart + "DNS Server:"         + tdBreak + htmlInput("text",     "dns_ip",        IPtoString(dns_ip))  + trEnd;
    httpData += trStart + "NTP Server 1:"       + tdBreak + htmlInput("text",     "ntp_server1",   ntp_server1)         + trEnd;
    httpData += trStart + "NTP Server 2:"       + tdBreak + htmlInput("text",     "ntp_server2",   ntp_server2)         + trEnd;

    httpData += thStart + "<b>MQTT:</b>" + thBreak + thEnd;

    httpData += trStart + "Server:"             + tdBreak + htmlInput("text",     "mqtt_server",   mqtt_server)   + trEnd;
    httpData += trStart + "Port:"               + tdBreak + htmlInput("text",     "mqtt_port",     mqtt_port)     + trEnd;
    httpData += trStart + "Name:"               + tdBreak + htmlInput("text",     "mqtt_name",     mqtt_name)     + trEnd;
    httpData += trStart + "Use TLS:"            + tdBreak + htmlCheckbox(         "mqtt_tls",      mqtt_tls)      + trEnd;
    httpData += trStart + "Use authentication:" + tdBreak + htmlCheckbox(         "mqtt_auth",     mqtt_auth)     + trEnd;
    httpData += trStart + "Username:"           + tdBreak + htmlInput("text",     "mqtt_user",     mqtt_user)     + trEnd;
    httpData += trStart + "Password:"           + tdBreak + htmlInput("text",     "mqtt_passwd",   mqtt_passwd)   + trEnd;
    httpData += trStart + "Data interval (s):"  + tdBreak + htmlInput("text",     "mqtt_interval", String(mqtt_interval)) + trEnd;
    httpData += trStart + "MQTT watchdog (s):"  + tdBreak + htmlInput("text",     "mqtt_watchdog", String(mqtt_watchdog)) + trEnd;

    httpData += thStart + "<b>Syslog:</b>" + thBreak + thEnd;

    httpData += trStart + "Use Syslog:"         + tdBreak + htmlCheckbox(         "use_syslog",    use_syslog)    + trEnd;
    httpData += trStart + "Server:"             + tdBreak + htmlInput("text",     "syslog_server", syslog_server) + trEnd;

    httpData += thStart + "<b>Sensors:</b>" + thBreak + thEnd;

    httpData += httpSensorSetup();
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

  httpServer.on("/password", [](){
    if (!is_authenticated()) {
      httpServer.sendHeader("Location","/login");
      httpServer.sendHeader("Cache-Control","no-cache");
      httpServer.send(301);
      return;
    }
    tempign = millis(); //reset the inactivity timer if someone logs in

    httpData = htmlHeader("/password");

    bool authChanged = false;
    if (httpServer.args()) {
      String currentPasswd = "";
      String newPasswd1    = "";
      String newPasswd2    = "";
      String newUser       = "";
      if ( httpServer.hasArg("currentPasswd") )
        currentPasswd = httpServer.arg("currentPasswd");
      if ( httpServer.hasArg("newPasswd1") )
        newPasswd1 = httpServer.arg("newPasswd1");
      if ( httpServer.hasArg("newPasswd2") )
        newPasswd2 = httpServer.arg("newPasswd2");
      if ( httpServer.hasArg("newUser") )
        newUser = httpServer.arg("newUser");
      if ( currentPasswd == httpPasswd ) {
        if ( newUser != "" ) {
          if ( newUser != httpUser ) {
            httpUser = newUser;
            httpData += "<p>Username changed.</p>";
            authChanged = true;
          }
        } else {
          httpData += "<p><b>Username empty. Not changed.</b></p>";
        }
        if ( newPasswd1 == newPasswd1 ) {
          if ( newPasswd1 != "" ) {
            if ( newPasswd1 != httpPasswd ) {
              httpPasswd = newPasswd1;
              httpData += "<p>Password changed.</p>";
              authChanged = true;
            } else {
              httpData += "<p><b>New password the same as old. Not changed.</b></p>";
            }
          } else {
            httpData += "<p><b>New password empty. Not changed.</b></p>";
          }
        } else {
          httpData += "<p><b>New passwords don't match. Not changed.</b></p>";
        }
      } else {
        httpData += "<p><b>Current password incorrent.</b></p>";
      }

      if (authChanged)
        saveConfig();
    }

    httpData += "<form action='#' method='POST'>";
    httpData += tableStart;

    httpData += thStart + "<b>Admin login:</b>" + thBreak + thEnd;

    httpData += trStart + "Username:"           + tdBreak + htmlInput("text",     "newUser",  httpUser) + trEnd;
    httpData += trStart + "Current password:"   + tdBreak + htmlInput("password", "currentPasswd", "")  + trEnd;
    httpData += trStart + tdBreak + trEnd;
    httpData += trStart + "New password:"       + tdBreak + htmlInput("password", "newPasswd1", "")     + trEnd;
    httpData += trStart + "Repeat new:"         + tdBreak + htmlInput("password", "newPasswd2", "")     + trEnd;

    httpData += trEnd;
    httpData += tableEnd;

    httpData += "<input type='submit' value='Submit'><input type='Reset'></form><br/>";

    httpData += httpFooter;
    httpServer.send(200, "text/html", httpData);
  });
}

String htmlHeader(String url) {
  String httpHeader = "<!DOCTYPE html><html><head><title>" + String(host_name) + ": " + String(fwname) + "</title></head><body><h1>" + String(host_name) + "</h1><h2>" + String(fwname) + "</h2><p>";
  if (url == "/") {
    httpHeader += "Home";
  } else {
    httpHeader += "<a href='/'>Home</a>";
  }
  httpHeader += " | ";
  if (url == "/config") {
    httpHeader += "Configuration";
  } else {
    httpHeader += "<a href='/config'>Configuration</a>";
  }
  httpHeader += " | ";
  if (url == "/firmware") {
    httpHeader += "Update firmware";
  } else {
    httpHeader += "<a href='/firmware'>Update firmware</a>";
  }
  httpHeader += " | ";
  if (url == "/system") {
    httpHeader += "System Info";
  } else {
    httpHeader += "<a href='/system'>System Info</a>";
  }
  httpHeader += " | ";
  if (url == "/reboot") {
    httpHeader += "Reboot";
  } else {
    httpHeader += "<a href='/reboot'>Reboot</a>";
  }
  httpHeader += " | ";
  if (url == "/password") {
    httpHeader += "Password";
  } else {
    httpHeader += "<a href='/password'>Password</a>";
  }
  httpHeader += " | ";
  httpHeader += "<a href='/logout'>Logout</a>";
  httpHeader += "</p>";

  if (rebootRequired) {
    httpHeader += "<p><b>Reboot required to use new settings.</b></p>";
  }

  return httpHeader;
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


void gencookie() {
  sessioncookie = "";
  for( uint8_t i = 0; i < 32; i++)
    sessioncookie += anchars[random(0, anchars.length())]; // Using random chars from anchars string generate 32-bit cookie 
}

bool is_authenticated() {
  // This function checks for Cookie header in request, it compares variable c to sessioncookie and returns true if they are the same
  if (httpServer.hasHeader("Cookie")) {
    String cookie = httpServer.header("Cookie"), authk = "c=" + sessioncookie;
    if (cookie.indexOf(authk) != -1)
      return true;
  }
  return false;
}
