static const String anchars = "abcdefghijklmnopqrstuvwxyz0123456789";        // Alfanumberic character filled string for cookie generation
String sessioncookie;                                                        // This is cookie buffer

const char HTTP_HEAD[] PROGMEM            = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/><title>{v}</title>";
const char HTTP_STYLE[] PROGMEM           = "<style>.c{text-align:center;} div,input{padding:5px;font-size:1em;} body{text-align:center;font-family:verdana;} table{text-align:left;} .nav li, button{border:0;border-radius:0.3rem;background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;} .q{float:right;width:90px;text-align:right;} ul.nav {padding: 0;} .nav li{display:inline-block;padding:0.5em;font-size:1rem;line-height:1.4rem;margin:0.2em;width:auto;} .nav li.active {background-color:#666;} .nav a{color:#fff;text-decoration:none;}</style>";
const char HTTP_SCRIPT[] PROGMEM          = "<script>function c(l){document.getElementById('s').value=l.innerText||l.textContent;document.getElementById('p').focus();}</script>";
const char HTTP_HEAD_END[] PROGMEM        = "</head><body><div style='text-align:left;display:inline-block;min-width:260px;'><h1>{t}</h1>";
const char HTTP_ITEM[] PROGMEM            = "<tr><td><a href='#p' onclick='c(this)'>{v}</a></td><td><span class='q {i}'>{r} dB</td></tr>";
const char HTTP_FORM_START[] PROGMEM      = "<form method='POST' action='wifisave'><input id='s' name='s' length=32 placeholder='SSID'><br/><input id='p' name='p' length=64 type='password' placeholder='password'><br/>";
const char HTTP_FORM_PARAM[] PROGMEM      = "<br/><input id='{i}' name='{n}' maxlength={l} placeholder='{p}' value='{v}' {c}>";
const char HTTP_FORM_END[] PROGMEM        = "<br/><button type='submit'>Save</button></form>";
const char HTTP_SCAN_LINK[] PROGMEM       = "<br/><div class=\"c\"><a href=\"/wifi\">Scan again</a></div>";
const char HTTP_SAVED[] PROGMEM           = "<div>Credentials Saved<br />Trying to connect ESP to network.<br />If it fails reconnect to AP to try again</div>";
const char HTTP_END[] PROGMEM             = "</div></body></html>";

const char HTTP_NAV_START[] PROGMEM       = "<ul class='nav'>";
const char HTTP_NAV_END[] PROGMEM         = "</ul>";
const char HTTP_NAV_LI[] PROGMEM          = "<li{c}>{l}</li>";

const char HTTP_META_REFRESH[] PROGMEM    = "<meta http-equiv='refresh' content='%u' />";

const char HTTP_H2_RED[] PROGMEM          = "<h2 style='color:red'>{v}</h2>";

const byte   MENU_COUNT = 10;
const String http_page_urls[MENU_COUNT]  = { "/", "/config", "/system", "/wifi", "/password", "/cacert", "/https", "/firmware", "/reboot", "/logout" };
const String http_page_names[MENU_COUNT] = { "Home", "Configuration", "Device Info", "WiFi", "Password", "CA Certificate", "HTTPS Certificate", "Update firmware", "Reboot", "Logout" };


File     fsUploadFile;           // a File object to temporarily store the received files
String   firmwareUpdateError;    // Contains any error message during firmware update
uint32_t uploadedFileSize;       // Conatins the size of any file uploaded

void httpSetup() {
  gencookie(); //generate new cookie on device start

  httpServer.on("/",             handleRoot );
  httpServer.on("/config",       handleConfig );
  httpServer.on("/system",       handleSystem );
  httpServer.on("/wifi",         handleWifi);
  httpServer.on("/wifisave",     handleWifiSave);
  httpServer.on("/generate_204", handleRoot);  // Android captive portal. Maybe not needed. Might be handled by notFound handler.   HTTP only
  httpServer.on("/fwlink",       handleRoot);  // Microsoft captive portal. Maybe not needed. Might be handled by notFound handler. HTTP only
  httpServer.on("/password",     handlePassword );
  httpServer.on("/cacert",       HTTP_GET,  handleCACert);
  httpServer.on("/cacert",       HTTP_POST, [](){ httpServer.send(200, "text/html", ""); }, handleCACertUpload);
  httpServer.on("/https",        HTTP_GET,  handleHTTPSCerts);
  httpServer.on("/httpscert",    HTTP_POST, [](){ httpServer.send(200, "text/html", ""); }, handleHTTPSCertUpload);
  httpServer.on("/httpskey",     HTTP_POST, [](){ httpServer.send(200, "text/html", ""); }, handleHTTPSKeyUpload);
  httpServer.on("/firmware",     HTTP_GET,  handleFirmware);
  httpServer.on("/firmware",     HTTP_POST, handleFirmwareUploadComplete,                   handleFirmwareUpload);
  httpServer.on("/reboot",       handleReboot );
  httpServer.on("/login",        handleLogin );
  httpServer.on("/logout",       handleLogout );
  httpServer.onNotFound(         handleNotFound );

}

String htmlHeader() {
  String httpHeader = FPSTR(HTTP_HEAD);
  httpHeader.replace("{v}", String(host_name) + ": " + String(fwname));
  httpHeader += FPSTR(HTTP_SCRIPT);
  httpHeader += FPSTR(HTTP_STYLE);
  httpHeader += FPSTR(HTTP_HEAD_END);
  httpHeader.replace("{t}", String(host_name) + " (" + String(fwname) + ")");

  if (httpServer.uri() != "/login") {
    httpHeader += FPSTR(HTTP_NAV_START);
    for (byte i = 0; i < MENU_COUNT; i++){
      String tmpString  = FPSTR(HTTP_NAV_LI);
      if (httpServer.uri() == http_page_urls[i]) {
        tmpString.replace("{c}", " class='active'");
        tmpString.replace("{l}", http_page_names[i]);
      } else {
        if ( (inConfigAP) && (http_page_urls[i] == "/logout") ) {
          tmpString = "";
        } else {
          tmpString.replace("{c}", "");
          tmpString.replace("{l}", "<a href='" + http_page_urls[i] + "'>" + http_page_names[i] + "</a>");
        }
      }
      httpHeader += tmpString;
    }
    httpHeader += FPSTR(HTTP_NAV_END);
  
    if (rebootRequired) {
      httpHeader += F("<p><b>Reboot required to use new settings.</b></p>");
    }
  }

  return httpHeader;
}

String htmlInput(String inputType, String inputName, String inputValue) {
  return String("<input type='" + inputType + "' name='" + inputName + "' value='" + inputValue + "'>");
}

String htmlCheckbox(String inputName, const bool inputChecked) {
  String tmpString = String(F("<input type='checkbox' name='")) + inputName + "'";
  if (inputChecked) {
    tmpString += " checked";
  }
  tmpString += ">";
  return tmpString;
}

String htmlRadio(String inputName, String inputValue, const bool inputChecked) {
  String tmpString = String(F("<input type='radio' name='")) + inputName + "' value='" + inputValue + "'";
  if (inputChecked) {
    tmpString += " checked";
  }
  tmpString += ">";
  return tmpString;
}

void sendLocationHeader(String loc) {
  httpServer.sendHeader(F("Location"),loc);         // Redirect the client to loc
}
void sendCacheControlHeader() {
  httpServer.sendHeader(F("Cache-Control"),F("no-cache"));
}
void sendCacheControlHeader(uint16_t len) {
  httpServer.sendHeader(F("Content-Length"), String(len));
}

void gencookie() {
  sessioncookie = "";
  for( uint8_t i = 0; i < 32; i++)
    sessioncookie += anchars[random(0, anchars.length())]; // Using random chars from anchars string generate 32-bit cookie 
}

bool is_authenticated() {
  if (inConfigAP)
    return true;
  // This function checks for Cookie header in request, it compares variable c to sessioncookie and returns true if they are the same
  if (httpServer.hasHeader("Cookie")) {
    String cookie = httpServer.header("Cookie"), authk = "c=" + sessioncookie;
    if (cookie.indexOf(authk) != -1)
      return true;
  }
  sendLocationHeader("/login");
  sendCacheControlHeader();
  httpServer.send(301);

  return false;
}


/** Is this an IP? */
boolean isIp(String str) {
  for (int i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean captivePortal() {
  if ( (inConfigAP) && (!isIp(httpServer.hostHeader()) ) ) { // The captive portal is always HTTP only so the httpServer object is the only one that needs to be looked at here
    Serial.println(F("Request redirected to captive portal"));
    sendLocationHeader(String("http://") + IPtoString(httpServer.client().localIP()));
    httpServer.send ( 302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    httpServer.client().stop(); // Stop is needed because we sent no content length
    return true;
  }
  return false;
}

void handleNotFound() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the error page.
    return;
  httpData = htmlHeader();
  httpData += F("404: not found: ");
  httpData += httpServer.uri();
  httpData += FPSTR(HTTP_END);
//  httpServer.sendHeader("Content-Length", String(httpData.length()));
  sendCacheControlHeader(httpData.length());
  httpServer.send(404, "text/html", httpData);
}

void handleLogin() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;

  httpData = htmlHeader();

  httpData += F("<div id=\"login\"><form action='/login' method='POST'>Login:<br/><input type='text' name='user' placeholder='User name'></p><p><input type='password' name='pass' placeholder='Password'></p><br/><button type='submit' name='submit'>login</button></center></form></body></html>");

  if (httpServer.hasHeader("Cookie")) {   
    String cookie = httpServer.header("Cookie"); // Copy the Cookie header to this buffer
  }

  // if user posted with these arguments
  if (httpServer.hasArg("user") && httpServer.hasArg("pass")) {
    // check if login details are good and dont allow it if device is in lockdown
    if (httpServer.arg("user") == httpUser && httpServer.arg("pass") == httpPasswd && !lock) {
      // if above values are good, send 'Cookie' header with variable c, with format 'c=sessioncookie'
      sendLocationHeader(http_page_urls[0]);  // "/"
      sendCacheControlHeader();
      httpServer.sendHeader("Set-Cookie","c=" + sessioncookie);
      httpServer.send(301);
      trycount = 0;                                 // With good headers in mind, reset the trycount buffer
      httpLoggedin = true;
      return;
    }

    httpData += "<center><br>";
    if (trycount != 10 && !lock)
      trycount++;                                   // If system is not locked up the trycount buffer
    if (trycount < 10 && !lock) {                   // We go here if systems isn't locked out, we give user 10 times to make a mistake after we lock down the system, thus making brute force attack almost imposible
      httpData += String(F("<p>Wrong username/password</p>You have "));
      httpData += (10 - trycount);
      httpData += String(F(" tries before system temporarily locks out."));
      logincld = millis();                          // Reset the logincld timer, since we still have available tries
    }
    
    if (trycount == 10) {                           // If too many bad tries
      httpData += String(F("Too many invalid login requests, you can try again in "));
      if (lock) {
        httpData += 5 - ((millis() - logincld) / 60000); // Display lock time remaining in minutes
      } else {
        logincld = millis();
        lock = true;
        httpData += "5";                                 // This happens when your device first locks down
      }
      httpData += " minutes.";
    }
  }
  httpData += "</center>";
  httpData += FPSTR(HTTP_END);
//  httpServer.sendHeader("Content-Length", String(httpData.length()));
  sendCacheControlHeader(httpData.length());
  httpServer.send(200, "text/html", httpData );
}

void handleLogout() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;

  //Set 'c=0', it users header, effectively deleting it's header
  httpServer.sendHeader("Set-Cookie","c=0");
  sendLocationHeader("/login");
  sendCacheControlHeader();
  httpServer.send(301);
  httpLoggedin = false;
  return;
}

void handleRoot() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;
  if (!is_authenticated()) {
    return;
  }
  tempign = millis(); //reset the inactivity timer if someone logs in
  char buf[50];

  httpData = htmlHeader();

  httpData += httpSensorData();
  httpData += "<br/>";

  /// send uptime and signal
  unsigned long uptime = millis();
  uptime = uptime / 1000;
  int signal = WiFi.RSSI();
  httpData += tableStart;
  httpData += trStart + "Uptime:" + tdBreak + String(uptime) + " s" + trEnd;
  httpData += trStart + "Signal:" + tdBreak + String(signal) + " dB" + trEnd;
  httpData += tableEnd;

  sprintf_P( buf, HTTP_META_REFRESH, 3);  // Refresh the page after 3 seconds
  httpData += String(buf);

  httpData += FPSTR(HTTP_END);
//  httpServer.sendHeader("Content-Length", String(httpData.length()));
  sendCacheControlHeader(httpData.length());
  httpServer.send(200, "text/html", httpData);
}


void handleSystem() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;
  if (!is_authenticated()) {
    return;
  }
  tempign = millis(); //reset the inactivity timer if someone logs in
  char buf[50];

  httpData = htmlHeader();

  httpData += tableStart;

  httpData += trStart + F("Firmware version:") + tdBreak + String(FWVERSION) + trEnd;

  httpData += trStart + String(FPSTR(str_nbsp)) + tdBreak + trEnd;

  httpData += trStart + F("MAC address:") + tdBreak;
  httpData += String(WiFi.macAddress());
  httpData += trEnd;

  httpData += trStart + String(FPSTR(str_nbsp)) + tdBreak + trEnd;

  httpData += trStart + F("Flash size:") + tdBreak;
  httpData += String(ESP.getFlashChipRealSize()/1048576.0) + " MB";
  httpData += trEnd;

  httpData += trStart + F("Flash size config:") + tdBreak;
  httpData += String(ESP.getFlashChipSize()/1048576.0) + " MB";
  httpData += trEnd;

  httpData += trStart + F("Program size:") + tdBreak;
  httpData += String(ESP.getSketchSize() / 1024) + " kB";
  httpData += trEnd;

  httpData += trStart + F("Free program size:") + tdBreak;
  httpData += String(ESP.getFreeSketchSpace() / 1024) + " kB";
  httpData += trEnd;

  httpData += trStart + String(FPSTR(str_nbsp)) + tdBreak + trEnd;

  httpData += trStart + F("Free memory:") + tdBreak;
  httpData += String(ESP.getFreeHeap() / 1024) + " kB";
  httpData += trEnd;

  httpData += trStart + F("ESP chip ID:") + tdBreak;
  httpData += String(ESP.getChipId());
  httpData += trEnd;

  httpData += trStart + F("Flash chip ID:") + tdBreak;
  httpData += String(ESP.getFlashChipId());
  httpData += trEnd;

  httpData += trStart + String(FPSTR(str_nbsp)) + tdBreak + trEnd;

  FSInfo fs_info;
  SPIFFS.info(fs_info);

  httpData += trStart + F("Filesystem size:") + tdBreak;
  httpData += String(fs_info.totalBytes / 1024) + " kB";
  httpData += trEnd;

  httpData += trStart + F("Filesystem free:") + tdBreak;
  httpData += String((fs_info.totalBytes - fs_info.usedBytes) / 1024) + " kB";
  httpData += trEnd;

  httpData += trStart + String(FPSTR(str_nbsp)) + tdBreak + trEnd;

  /// send uptime and signal
  unsigned long uptime = millis();
  uptime = uptime / 1000;
  httpData += trStart + F("Uptime:") + tdBreak + String(uptime) + " s" + trEnd;

  httpData += trStart + String(FPSTR(str_nbsp)) + tdBreak + trEnd;

  httpData += trStart + F("SSID:")   + tdBreak + String(ssid) + trEnd;
  httpData += trStart + F("Signal:") + tdBreak + String(WiFi.RSSI()) + " dB" + trEnd;
  httpData += trStart + F("IP:")     + tdBreak + IPtoString(WiFi.localIP()) + trEnd;
  httpData += trStart + String(FPSTR(str_nbsp)) + tdBreak + trEnd;
  httpData += tableEnd;

  sprintf_P( buf, HTTP_META_REFRESH, 3);  // Refresh the page after 3 seconds
  httpData += String(buf);
  httpData += FPSTR(HTTP_END);

//  httpServer.sendHeader("Content-Length", String(httpData.length()));
  sendCacheControlHeader(httpData.length());
  httpServer.send(200, "text/html", httpData);
}


void handleConfig() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;
  if (!is_authenticated()) {
    return;
  }
  tempign = millis(); //reset the inactivity timer if someone logs in

  httpData = htmlHeader();

  if (httpServer.args()) {
    IPAddress tmpip;

    if (httpServer.hasArg("staticip")) {
      if (httpServer.arg("staticip") == String(FPSTR(str_true))) {
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

    if (httpServer.hasArg(String(FPSTR(cfg_ip_address)))) {
      tmpip.fromString(httpServer.arg(String(FPSTR(cfg_ip_address))));
      if (ip != tmpip) {
        if (use_staticip)
          rebootRequired = true;        // Changing IP requires a reboot
        ip = tmpip;
      }
    }
    if (httpServer.hasArg(String(FPSTR(cfg_subnet)))) {
      tmpip.fromString(httpServer.arg(String(FPSTR(cfg_subnet))));
      if (subnet != tmpip) {
        if (use_staticip)
          rebootRequired = true;        // Changing subnet requires a reboot
        subnet = tmpip;
      }
    }
    if (httpServer.hasArg(String(FPSTR(cfg_gateway)))) {
      tmpip.fromString(httpServer.arg(String(FPSTR(cfg_gateway))));
      if (gateway != tmpip) {
        if (use_staticip)
          rebootRequired = true;        // Changing gateway IP requires a reboot
        gateway = tmpip;
      }
    }
    if (httpServer.hasArg(String(FPSTR(cfg_dns_server)))) {
      tmpip.fromString(httpServer.arg(String(FPSTR(cfg_dns_server))));
      if (dns_ip != tmpip) {
        if (use_staticip)
          rebootRequired = true;        // Changing DNS server requires a reboot
        dns_ip = tmpip;
      }
    }
    if (httpServer.hasArg(String(FPSTR(cfg_ntp_server1)))) {
      tmpString = String(ntp_server1);
      if (httpServer.arg(String(FPSTR(cfg_ntp_server1))) != tmpString) {
        rebootRequired = true;          // Changing NTP servers requires a reboot
        httpServer.arg(String(FPSTR(cfg_ntp_server1))).toCharArray(ntp_server1, 40);
      }
    }
    if (httpServer.hasArg(String(FPSTR(cfg_ntp_server2)))) {
      tmpString = String(ntp_server2);
      if (httpServer.arg(String(FPSTR(cfg_ntp_server2))) != tmpString) {
        rebootRequired = true;          // Changing NTP servers requires a reboot
        httpServer.arg(String(FPSTR(cfg_ntp_server2))).toCharArray(ntp_server2, 40);
      }
    }

    if (httpServer.hasArg(String(FPSTR(cfg_mqtt_server)))) {
      tmpString = String(mqtt_server);
      if (httpServer.arg(String(FPSTR(cfg_mqtt_server))) != tmpString) {
        httpServer.arg(String(FPSTR(cfg_mqtt_server))).toCharArray(mqtt_server, 40);
        mqttClient.disconnect();        // MQTT will reconnect automatically with the new value
        mqttClient.setServer(mqtt_server, atoi( mqtt_port ));
      }
    }
    if (httpServer.hasArg(String(FPSTR(cfg_mqtt_port)))) {
      tmpString = String(mqtt_port);
      if (httpServer.arg(String(FPSTR(cfg_mqtt_port))) != tmpString) {
        httpServer.arg(String(FPSTR(cfg_mqtt_port))).toCharArray(mqtt_port, 5);
        mqttClient.disconnect();        // MQTT will reconnect automatically with the new value
        mqttClient.setServer(mqtt_server, atoi( mqtt_port ));
      }
    }
    if (httpServer.hasArg(String(FPSTR(cfg_mqtt_name)))) {
      tmpString = String(mqtt_name);
      if (httpServer.arg(String(FPSTR(cfg_mqtt_name))) != tmpString) {
        mqttClient.disconnect();        // MQTT will reconnect automatically with the new value
        httpServer.arg(String(FPSTR(cfg_mqtt_name))).toCharArray(mqtt_name, 20);
      }
    }
    if (httpServer.hasArg(String(FPSTR(cfg_mqtt_tls)))) {
      if (httpServer.arg(String(FPSTR(cfg_mqtt_tls))) == "on") {
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
    if (httpServer.hasArg(String(FPSTR(cfg_mqtt_auth)))) {
      if (httpServer.arg(String(FPSTR(cfg_mqtt_auth))) == "on") {
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
    if (httpServer.hasArg(String(FPSTR(cfg_mqtt_user)))) {
      tmpString = String(mqtt_user);
      if (httpServer.arg(String(FPSTR(cfg_mqtt_user))) != tmpString) {
        mqttClient.disconnect();        // MQTT will reconnect automatically with the new value
        httpServer.arg(String(FPSTR(cfg_mqtt_user))).toCharArray(mqtt_user, 20);
      }
    }
    if (httpServer.hasArg(String(FPSTR(cfg_mqtt_passwd)))) {
      tmpString = String(mqtt_passwd);
      if (httpServer.arg(String(FPSTR(cfg_mqtt_passwd))) != tmpString) {
        mqttClient.disconnect();        // MQTT will reconnect automatically with the new value
        httpServer.arg(String(FPSTR(cfg_mqtt_passwd))).toCharArray(mqtt_passwd, 32);
      }
    }

    if (httpServer.hasArg(String(FPSTR(cfg_mqtt_interval))))
      mqtt_interval = httpServer.arg(String(FPSTR(cfg_mqtt_interval))).toInt();

    if (httpServer.hasArg(String(FPSTR(cfg_mqtt_watchdog)))) {
      mqtt_watchdog = httpServer.arg(String(FPSTR(cfg_mqtt_watchdog))).toInt();
      if (mqtt_watchdog <= mqtt_interval) {
        mqtt_watchdog = mqtt_interval + 1;
      }
      if (mqtt_watchdog < 60) {
        mqtt_watchdog = 60;
      }
    }

    if (httpServer.hasArg(String(FPSTR(cfg_syslog_server))))
      httpServer.arg(String(FPSTR(cfg_syslog_server))).toCharArray(syslog_server, 40);
    if (httpServer.hasArg(String(FPSTR(cfg_use_syslog)))) {
      if (httpServer.arg(String(FPSTR(cfg_use_syslog))) == "on") {
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

  httpData += F("<form action='#' method='POST'>");
  httpData += tableStart;

  httpData += thStart + F("Network:") + thBreak + thEnd;
  httpData += trStart + "IP:"                 + tdBreak;
  httpData += htmlRadio("staticip", String(FPSTR(str_true)),          use_staticip)     + "Static IP";
  httpData += htmlRadio("staticip", String(FPSTR(str_false)),         (! use_staticip)) + "DHCP";
  httpData += trEnd;
  httpData += trStart + "Address:"            + tdBreak + htmlInput("text",     String(FPSTR(cfg_ip_address)),    IPtoString(ip))      + trEnd;
  httpData += trStart + "Subnet Mask:"        + tdBreak + htmlInput("text",     String(FPSTR(cfg_subnet)),        IPtoString(subnet))  + trEnd;
  httpData += trStart + "Gateway:"            + tdBreak + htmlInput("text",     String(FPSTR(cfg_gateway)),       IPtoString(gateway)) + trEnd;
  httpData += trStart + "DNS Server:"         + tdBreak + htmlInput("text",     String(FPSTR(cfg_dns_server)),    IPtoString(dns_ip))  + trEnd;
  httpData += trStart + "NTP Server 1:"       + tdBreak + htmlInput("text",     String(FPSTR(cfg_ntp_server1)),   ntp_server1)         + trEnd;
  httpData += trStart + "NTP Server 2:"       + tdBreak + htmlInput("text",     String(FPSTR(cfg_ntp_server2)),   ntp_server2)         + trEnd;

  httpData += thStart + "MQTT:" + thBreak + thEnd;

  httpData += trStart + "Server:"             + tdBreak + htmlInput("text",     String(FPSTR(cfg_mqtt_server)),   mqtt_server)   + trEnd;
  httpData += trStart + "Port:"               + tdBreak + htmlInput("text",     String(FPSTR(cfg_mqtt_port)),     mqtt_port)     + trEnd;
  httpData += trStart + "Name:"               + tdBreak + htmlInput("text",     String(FPSTR(cfg_mqtt_name)),     mqtt_name)     + trEnd;
  httpData += trStart + "Use TLS:"            + tdBreak + htmlCheckbox(         String(FPSTR(cfg_mqtt_tls)),      mqtt_tls)      + trEnd;
  httpData += trStart + "Use authentication:" + tdBreak + htmlCheckbox(         String(FPSTR(cfg_mqtt_auth)),     mqtt_auth)     + trEnd;
  httpData += trStart + "Username:"           + tdBreak + htmlInput("text",     String(FPSTR(cfg_mqtt_user)),     mqtt_user)     + trEnd;
  httpData += trStart + "Password:"           + tdBreak + htmlInput("text",     String(FPSTR(cfg_mqtt_passwd)),   mqtt_passwd)   + trEnd;
  httpData += trStart + "Data interval (s):"  + tdBreak + htmlInput("text",     String(FPSTR(cfg_mqtt_interval)), String(mqtt_interval)) + trEnd;
  httpData += trStart + "MQTT watchdog (s):"  + tdBreak + htmlInput("text",     String(FPSTR(cfg_mqtt_watchdog)), String(mqtt_watchdog)) + trEnd;

  httpData += thStart + "Syslog:" + thBreak + thEnd;

  httpData += trStart + "Use Syslog:"         + tdBreak + htmlCheckbox(         String(FPSTR(cfg_use_syslog)),    use_syslog)    + trEnd;
  httpData += trStart + "Server:"             + tdBreak + htmlInput("text",     String(FPSTR(cfg_syslog_server)), syslog_server) + trEnd;

  httpData += thStart + "Sensors:" + thBreak + thEnd;

  httpData += httpSensorSetup();

  httpData += trEnd;
  httpData += tableEnd;

  httpData += FPSTR(HTTP_FORM_END);

  httpData += FPSTR(HTTP_END);
//  httpServer.sendHeader("Content-Length", String(httpData.length()));
  sendCacheControlHeader(httpData.length());
  httpServer.send(200, "text/html", httpData);
}


void handlePassword() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;
  if (!is_authenticated()) {
    return;
  }
  tempign = millis(); //reset the inactivity timer if someone logs in

  httpData = htmlHeader();

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
          logString = "Username changed";
          mqttLog(logString);
        }
      } else {
        httpData += String(F("<p><b>Username empty. Not changed.</b></p>"));
      }
      if ( newPasswd1 == newPasswd1 ) {
        if ( newPasswd1 != "" ) {
          if ( newPasswd1 != httpPasswd ) {
            httpPasswd = newPasswd1;
            httpData += String(F("<p>Password changed.</p>"));
            authChanged = true;
            logString = "Password changed";
            mqttLog(logString);
          } else {
            httpData += String(F("<p><b>New password the same as old. Not changed.</b></p>"));
          }
        } else {
          httpData += String(F("<p><b>New password empty. Not changed.</b></p>"));
        }
      } else {
        httpData += String(F("<p><b>New passwords don't match. Not changed.</b></p>"));
      }
    } else {
      httpData += String(F("<p><b>Current password incorrent.</b></p>"));
    }

    if (authChanged)
      saveConfig();
  }

  httpData += String(F("<form action='#' method='POST'>"));
  httpData += tableStart;

  httpData += thStart + String(F("Admin login:")) + thBreak + thEnd;

  httpData += trStart + "Username:"           + tdBreak + htmlInput("text",     "newUser",  httpUser) + trEnd;
  httpData += trStart + "Current password:"   + tdBreak + htmlInput("password", "currentPasswd", "")  + trEnd;
  httpData += trStart + String(FPSTR(str_nbsp)) + tdBreak + trEnd;
  httpData += trStart + "New password:"       + tdBreak + htmlInput("password", "newPasswd1", "")     + trEnd;
  httpData += trStart + "Repeat new:"         + tdBreak + htmlInput("password", "newPasswd2", "")     + trEnd;

  httpData += trEnd;
  httpData += tableEnd;

  httpData += FPSTR(HTTP_FORM_END);

  httpData += FPSTR(HTTP_END);
//  httpServer.sendHeader("Content-Length", String(httpData.length()));
  sendCacheControlHeader(httpData.length());
  httpServer.send(200, "text/html", httpData);
}



/** Wifi config page handler */
void handleWifi() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;
  if (!is_authenticated()) {
    return;
  }
  tempign = millis(); //reset the inactivity timer if someone logs in

  httpData = htmlHeader();

  if (! inConfigAP) {
    httpData += FPSTR(HTTP_H2_RED);
    httpData.replace("{v}", F("Warning"));
    httpData += (String(F("<p>You are connected through the wifi network: ")) + ssid + "<br/>");
    httpData += (String(F("<b>If you change WiFi networks you may not be able to reconnect to the device.</b></p>")));
  }

  int signal = WiFi.RSSI();
  ssid = WiFi.SSID();
  httpData += tableStart;
  httpData += thStart + String(F("Current config:")) + thBreak + thEnd;
  httpData += trStart + "SSID:"   + tdBreak + String(ssid) + trEnd;
  if (! inConfigAP) {
    httpData += trStart + "Signal:" + tdBreak + String(signal) + " dB" + trEnd;
    httpData += trStart + "IP:"     + tdBreak + IPtoString(WiFi.localIP()) + trEnd;
  }
  httpData += trEnd;
  httpData += tableEnd;

  httpData += "<br/>";

//  logString = String(F("Web server available by http://")) + String(host_name) + F(".local/");


  logString = F("scan start");
  logMessage(app_name_wifi, logString, true);
  int n = WiFi.scanNetworks();
  logString = F("scan done");
  logMessage(app_name_wifi, logString, true);

  if (n == 0) {
    httpData += F("No networks found.");
  } else {
    httpData += F("WiFi network list:");

    //sort networks
    int indices[n];
    for (int i = 0; i < n; i++) {
      indices[i] = i;
    }
  
    // RSSI SORT
  
    // old sort
    for (int i = 0; i < n; i++) {
      for (int j = i + 1; j < n; j++) {
        if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
          std::swap(indices[i], indices[j]);
        }
      }
    }

    // Remove duplicate SSIDs
    String cssid;
    for (int i = 0; i < n; i++) {
      if (indices[i] == -1) continue;
      cssid = WiFi.SSID(indices[i]);
      for (int j = i + 1; j < n; j++) {
        if (cssid == WiFi.SSID(indices[j])) {
          indices[j] = -1; // set dup SSIDs to index -1
        }
      }
    }

    httpData += tableStart;
    for (int i = 0; i < n; i++) {
      if (indices[i] == -1)
        continue; // skip dups

      String item = FPSTR(HTTP_ITEM);
      item.replace("{v}", WiFi.SSID(indices[i]));
      item.replace("{r}", String(WiFi.RSSI(indices[i])));
      if (WiFi.encryptionType(indices[i]) != ENC_TYPE_NONE) {
        item.replace("{i}", "l");
      } else {
        item.replace("{i}", "");
      }
      httpData += item;
      delay(0);
    }
    httpData += tableEnd;
  }


  httpData += FPSTR(HTTP_FORM_START);
  httpData += FPSTR(HTTP_FORM_END);
  httpData += FPSTR(HTTP_SCAN_LINK);

  httpData += FPSTR(HTTP_END);

//  httpServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  sendCacheControlHeader();     // Does this need to send "no-cache, no-store, must-revalidate"?
  httpServer.sendHeader("Pragma", "no-cache");
  httpServer.sendHeader("Expires", "-1");
//  httpServer.sendHeader("Content-Length", String(httpData.length()));
  sendCacheControlHeader(httpData.length());
  httpServer.send(200, "text/html", httpData);
}

/** Handle the WLAN save form and redirect to WLAN config page again */
void handleWifiSave() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;
  if (!is_authenticated()) {
    return;
  }
  tempign = millis(); //reset the inactivity timer if someone logs in
  char buf[50];

  // Store new credentials here for checking later
  _ssid = httpServer.arg("s");
  _psk  = httpServer.arg("p");

  if (_ssid != "" && _psk != "") {
    httpData = FPSTR(HTTP_HEAD);
    httpData.replace("{v}", "Credentials Saved");
    httpData += FPSTR(HTTP_SCRIPT);
    httpData += FPSTR(HTTP_STYLE);
    httpData += FPSTR(HTTP_HEAD_END);
    httpData.replace("{t}", String(host_name) + " (" + String(fwname) + ")");
    httpData += FPSTR(HTTP_SAVED);
    httpData += FPSTR(HTTP_END);

    sprintf_P( buf, HTTP_META_REFRESH, 10);  // Refresh the page after 10 seconds
    httpData += String(buf);
    
    connectNewWifi = true; //signal ready to connect/reset
  } else {
    httpData = htmlHeader();
    httpData += F("Missing WiFi network or password.");

    sprintf_P( buf, HTTP_META_REFRESH, 3);  // Refresh the page after 3 seconds
    httpData += String(buf);

    httpData += FPSTR(HTTP_END);
  }
//  httpServer.sendHeader("Content-Length", String(httpData.length()));
  sendCacheControlHeader(httpData.length());
  httpServer.send(200, "text/html", httpData);
}


//////////////////////////////////// upload pages ////////////////////////////////////


void handleFirmware() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;
  if (!is_authenticated()) {
    return;
  }
  tempign = millis();  // reset the inactivity timer if someone logs in

  httpData = htmlHeader();

  httpData += F("Select a firmware file to upload.<br />Be certain to make sure it's compiled with the same SPIFFS size or the next boot will fail.<br/>");
  httpData += F("<form action='#' method='POST' enctype='multipart/form-data'>");
  httpData += F("<input type='file' name='firmware'>");
  httpData += F("<br/><button type='submit'>Update</button></form>");

  httpData += FPSTR(HTTP_END);
//  httpServer.sendHeader("Content-Length", String(httpData.length()));
  sendCacheControlHeader(httpData.length());
  httpServer.send(200, "text/html", httpData);
}

void handleFirmwareUploadComplete() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;
  if (!is_authenticated()) {
    return;
  }
  tempign = millis();  // reset the inactivity timer if someone logs in
  char buf[50];

  httpData = "</p>";   // Finishes the progress bar

  if (Update.hasError() || firmwareUpdateError != "") {
    httpData += FPSTR(HTTP_H2_RED);
    httpData.replace("{v}", str_firmware_update + F("failed."));

    httpData += String(F("<pre>")) + firmwareUpdateError + F("</pre>");
  
    httpData += FPSTR(HTTP_END);
    httpServer.sendContent(httpData);
    httpServer.client().stop(); // Stop is needed because we sent no content length

    // We should restart WiFiUDP here
  } else {
    sprintf_P( buf, HTTP_META_REFRESH, 15);  // Refresh the page after 15 seconds
    httpData += String(buf);

    httpData += String(F("<p>")) + str_firmware_update + F("Success, ");
    httpData += String(uploadedFileSize) + F(" bytes uploaded.<br/>") + str_rebooting + F("</p></body></html>");
    httpServer.client().setNoDelay(true);
    httpServer.sendContent(httpData);
    delay(100);
    httpServer.client().stop();
    delay(100);
    ESP.restart();
  }
}

void handleFirmwareUpload() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;
  if (!is_authenticated()) {
    return;
  }
  tempign = millis(); //reset the inactivity timer if someone logs in

  HTTPUpload& upload = httpServer.upload();

  if (upload.status == UPLOAD_FILE_START) {
    firmwareUpdateError = "";
    uploadedFileSize    = 0;
    Serial.setDebugOutput(true);
    logString = str_firmware_update + upload.filename;
    logMessage(app_name_sys, logString, true);
    delay(100);         // Give time for the syslog packet to be sent by UDP
    WiFiUDP::stopAll();

    httpServer.client().setNoDelay(true);
    httpServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
    httpServer.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.

    httpData = htmlHeader();
    httpData += F("<p style='background-color:#0A0;width:fit-content;font-size:5px;'>");

    httpServer.sendContent(httpData);

    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if (!Update.begin(maxSketchSpace)) {                  // start with max available size
      Update.printError(Serial);
      StreamString str;
      Update.printError(str);
      firmwareUpdateError = str.c_str();
    }
  } else if (upload.status == UPLOAD_FILE_WRITE && !firmwareUpdateError.length()) {
    Serial.print(".");
    httpServer.sendContent("_");

    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize){
      Update.printError(Serial);
      StreamString str;
      Update.printError(str);
      firmwareUpdateError = str.c_str();
      logString = str_firmware_update + firmwareUpdateError;
      logMessage(app_name_sys, logString, true);
    }
  } else if (upload.status == UPLOAD_FILE_END && !firmwareUpdateError.length()) {
    if (Update.end(true)) {                               // true to set the size to the current progress
      uploadedFileSize = upload.totalSize;
      if (upload.totalSize != 0 ) {
        Serial.printf_P( PSTR("\nUpdate Success: %u bytes\nReboot required...\n"), upload.totalSize);
      } else {
        firmwareUpdateError = F("no data uploaded.");
        logString = str_firmware_update + firmwareUpdateError;
        logMessage(app_name_sys, logString, true);
      }
    } else {
      Update.printError(Serial);
      StreamString str;
      Update.printError(str);
      firmwareUpdateError = str.c_str();
      logString = str_firmware_update + firmwareUpdateError;
      logMessage(app_name_sys, logString, true);
    }
    Serial.setDebugOutput(false);
  } else if(upload.status == UPLOAD_FILE_ABORTED) {
    Update.end();
    firmwareUpdateError = F("Update was aborted");
    logString = str_firmware_update + firmwareUpdateError;
    logMessage(app_name_sys, logString, true);
  }
  delay(0);
}

void handleReboot() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;
  if (!is_authenticated()) {
    return;
  }
  tempign = millis(); //reset the inactivity timer if someone logs in
  char buf[50];

  httpData = htmlHeader();
  sprintf_P( buf, HTTP_META_REFRESH, 10);  // Refresh the page after 10 seconds
  httpData += String(buf);
  httpData += str_rebooting;
  httpData += FPSTR(HTTP_END);

//  httpServer.sendHeader("Content-Length", String(httpData.length()));
  sendCacheControlHeader(httpData.length());
  httpServer.send(200, "text/html", httpData);
  delay(100);
  ESP.restart();
  delay(5000);
}


/** Handle the CA certificate upload page */
void handleCACert() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;
  if (!is_authenticated()) {
    return;
  }
  tempign = millis(); //reset the inactivity timer if someone logs in

  httpData = htmlHeader();

  httpData += F("<p>This CA certificate is for the secure TLS MQTT connection.</p>");

  if ( SPIFFS.exists(CAcertfilename) ) {
    File certfile = SPIFFS.open(CAcertfilename, "r");
    httpData += FPSTR(HTTP_H2_RED);
    httpData.replace("{v}", F("Existing certificate"));
    httpData += F("There is an existing CA certificate with a size of ");
    httpData += certfile.size();
    httpData += F(" bytes<hr>");
  }

  httpData += F("Select a file to upload with the CA certificate in DER format.<br/>");
  httpData += F("A certificate can be converted to DER format with openssl with the following command edited as appropriate:");
  httpData += F("<pre>openssl x509 -in CACert.pem -out CACert.der -outform DER</pre>");
  httpData += F("<form action='#' method='POST' enctype='multipart/form-data'>");
  httpData += F("<input type='file' name='cacert'>");
  httpData += F("<br/><button type='submit'>Upload</button></form>");

  httpData += FPSTR(HTTP_END);
//  httpServer.sendHeader("Content-Length", String(httpData.length()));
  sendCacheControlHeader(httpData.length());
  httpServer.send(200, "text/html", httpData);
}

/* Handle a file being uploaded */
void handleCACertUpload() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;
  if (!is_authenticated()) {
    return;
  }
  tempign = millis(); //reset the inactivity timer if someone logs in

  HTTPUpload& upload = httpServer.upload();

  if (upload.status == UPLOAD_FILE_START) {
    if ( SPIFFS.exists(CAcertfilename) )
      SPIFFS.remove(CAcertfilename);
    fsUploadFile = SPIFFS.open(CAcertfilename, "w");      // Save the file as cacert.der no matter the supplied filename
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {                                   // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      sendLocationHeader(http_page_urls[5]);              // Redirect the client to the "/cacert" page
      httpServer.send(303);
    } else {
      httpData = htmlHeader();
      httpData += F("500: couldn't create file");
      httpData += FPSTR(HTTP_END);
      httpServer.send(500, "text/html", httpData );
    }
  }
}


/** Handle the HTTPS certificate and key upload page */
void handleHTTPSCerts() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;
  if (!is_authenticated()) {
    return;
  }
  tempign = millis(); //reset the inactivity timer if someone logs in

  httpData = htmlHeader();

  httpData += FPSTR(HTTP_H2_RED);
  httpData.replace("{v}", F("Currently unused!"));
  httpData += F("<p>This certificate and key is for the secure HTTPS interface.<br/>It is not needed for MQTT but is a needed for a secure connection to this device's web interface.</p>");

  httpData += F("The files need to be uploaded in DER format.<br/>");
  httpData += F("A certificate can be converted to DER format with openssl with the following command edited as appropriate:");
  httpData += F("<pre>openssl x509 -in filename.pem -out filename.pem.der -outform DER</pre>");
  httpData += F("and the key can be converted with:");
  httpData += F("<pre>openssl rsa -in filename.key -out filename.key.der -outform DER</pre>");

  httpData += tableStart;
  httpData += trStart;

  if ( SPIFFS.exists(HTTPScertfilename) ) {
    File certfile = SPIFFS.open(HTTPScertfilename, "r");
    httpData += FPSTR(HTTP_H2_RED);
    httpData.replace("{v}", F("Existing certificate"));
    httpData += F("There is an existing HTTPS certificate with a size of ");
    httpData += certfile.size();
    httpData += F(" bytes<hr>");
  }
  httpData += F("<form action='/httpscert' method='POST' enctype='multipart/form-data'>");
  httpData += F("<input type='file' name='httpscert'>");
  httpData += F("<br/><button type='submit'>Upload certificate</button></form>");
  httpData += tdBreak;

  if ( SPIFFS.exists(HTTPSkeyfilename) ) {
    File certfile = SPIFFS.open(HTTPSkeyfilename, "r");
    httpData += FPSTR(HTTP_H2_RED);
    httpData.replace("{v}", F("Existing key"));
    httpData += F("There is an existing HTTPS key with a size of ");
    httpData += certfile.size();
    httpData += F(" bytes<hr>");
  }
  httpData += F("<form action='/httpskey' method='POST' enctype='multipart/form-data'>");
  httpData += F("<input type='file' name='httpskey'>");
  httpData += F("<br/><button type='submit'>Upload key</button></form>");
  httpData += trEnd;
  httpData += tableEnd;

  httpData += FPSTR(HTTP_END);
//  httpServer.sendHeader("Content-Length", String(httpData.length()));
  sendCacheControlHeader(httpData.length());
  httpServer.send(200, "text/html", httpData);
}

/* Handle the cert file being uploaded */
void handleHTTPSCertUpload() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;
  if (!is_authenticated()) {
    return;
  }
  tempign = millis(); //reset the inactivity timer if someone logs in

  HTTPUpload& upload = httpServer.upload();

  if (upload.status == UPLOAD_FILE_START) {
    if ( SPIFFS.exists(HTTPScertfilename) )
      SPIFFS.remove(HTTPScertfilename);
    fsUploadFile = SPIFFS.open(HTTPScertfilename, "w");   // Save the HTTPS certificate file
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {                                   // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      sendLocationHeader(http_page_urls[6]);              // Redirect the client to the "/https" page
      httpServer.send(303);
    } else {
      httpData = htmlHeader();
      httpData += F("500: couldn't create file");
      httpData += FPSTR(HTTP_END);
      httpServer.send(500, "text/html", httpData );
    }
  }
}
/* Handle the key file being uploaded */
void handleHTTPSKeyUpload() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;
  if (!is_authenticated()) {
    return;
  }
  tempign = millis(); //reset the inactivity timer if someone logs in

  HTTPUpload& upload = httpServer.upload();

  if (upload.status == UPLOAD_FILE_START) {
    if ( SPIFFS.exists(HTTPSkeyfilename) )
      SPIFFS.remove(HTTPSkeyfilename);
    fsUploadFile = SPIFFS.open(HTTPSkeyfilename, "w");   // Save the HTTPS key file
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {                                   // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      sendLocationHeader(http_page_urls[6]);              // Redirect the client to the "/https" page
      httpServer.send(303);
    } else {
      httpData = htmlHeader();
      httpData += F("500: couldn't create file");
      httpData += FPSTR(HTTP_END);
      httpServer.send(500, "text/html", httpData );
    }
  }
}
