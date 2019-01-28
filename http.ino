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

const String str_h2_red                   = "<h2 style='color:red'>{v}</h2>";
const String str_tick_green               = "<span style='color:green'>&#10004;</span>";
const String str_cross_red                = "<span style='color:red'>&#10008;</span>";

const byte   MENU_COUNT = 10;
const String http_page_urls[MENU_COUNT]  = { "/", "/config", "/system", "/wifi", "/password", "/cacert", "/https", "/firmware", "/reboot", "/logout" };
const String http_page_names[MENU_COUNT] = { "Home", "Configuration", "Device Info", "WiFi", "Password", "CA Certificate", "HTTPS Certificate", "Update firmware", "Reboot", "Logout" };
const String login_url                   = "/login";


File     fsUploadFile;           // a File object to temporarily store the received files
String   firmwareUpdateError;    // Contains any error message during firmware update
uint32_t uploadedFileSize;       // Conatins the size of any file uploaded

void httpSetup() {
  gencookie(); //generate new cookie on device start

  httpServer.on(http_page_urls[0], handleRoot );
  httpServer.on(http_page_urls[1], handleConfig );
  httpServer.on(http_page_urls[2], handleSystem );
  httpServer.on(http_page_urls[3], handleWifi);
  httpServer.on("/wifisave",       handleWifiSave);
  httpServer.on("/generate_204",   handleRoot);  // Android captive portal. Maybe not needed. Might be handled by notFound handler.   HTTP only
  httpServer.on("/fwlink",         handleRoot);  // Microsoft captive portal. Maybe not needed. Might be handled by notFound handler. HTTP only
  httpServer.on(http_page_urls[4], handlePassword );
  httpServer.on(http_page_urls[5], HTTP_GET,  handleCACert);
  httpServer.on(http_page_urls[5], HTTP_POST, []() {
    send200Http("");
  },     handleCACertUpload);
  httpServer.on(http_page_urls[6], HTTP_GET,  handleHTTPSCerts);
  httpServer.on("/httpscert",      HTTP_POST, []() {
    send200Http("");
  },     handleHTTPSCertUpload);
  httpServer.on("/httpskey",       HTTP_POST, []() {
    send200Http("");
  },     handleHTTPSKeyUpload);
  httpServer.on(http_page_urls[7], HTTP_GET,  handleFirmware);
  httpServer.on(http_page_urls[7], HTTP_POST, handleFirmwareUploadComplete, handleFirmwareUpload);
  httpServer.on(http_page_urls[8], handleReboot );
  httpServer.on(login_url,         handleLogin );
  httpServer.on(http_page_urls[9], handleLogout );
  httpServer.onNotFound(           handleNotFound );

}

String htmlHeader() {
  String httpHeader = FPSTR(HTTP_HEAD);
  httpHeader.replace("{v}", String(host_name) + ": " + String(fwname));
  httpHeader += FPSTR(HTTP_SCRIPT);
  httpHeader += FPSTR(HTTP_STYLE);
  httpHeader += FPSTR(HTTP_HEAD_END);
//  httpHeader.replace("{t}", String(host_name) + " (" + String(fwname) + ")");
  httpHeader.replace("{t}", String(host_name));

  if (httpServer.uri() != login_url) {
    httpHeader += FPSTR(HTTP_NAV_START);
    for (byte i = 0; i < MENU_COUNT; i++) {
      String tmpString  = FPSTR(HTTP_NAV_LI);
      if (httpServer.uri() == http_page_urls[i]) {
        tmpString.replace("{c}", F(" class='active'"));
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

String htmlInput(const String &inputType, const String &inputName, const String &inputValue) {
  return String("<input type='" + inputType + "' name='" + inputName + "' value='" + inputValue + "'>");
}

String htmlCheckbox(const String &inputName, const bool inputChecked) {
  String tmpString = String(F("<input type='checkbox' name='")) + inputName + "'";
  if (inputChecked) {
    tmpString += " checked";
  }
  tmpString += ">";
  return tmpString;
}

String htmlRadio(const String &inputName, const String &inputValue, const bool inputChecked) {
  String tmpString = String(F("<input type='radio' name='")) + inputName + "' value='" + inputValue + "'";
  if (inputChecked) {
    tmpString += " checked";
  }
  tmpString += ">";
  return tmpString;
}

void sendLocationHeader(const String &loc) {
  httpServer.sendHeader(F("Location"), loc);        // Redirect the client to loc
}
void sendCacheControlHeader() {
  httpServer.sendHeader(F("Cache-Control"), F("no-cache"));
}
void sendCacheControlHeader(uint16_t len) {
  httpServer.sendHeader(F("Content-Length"), String(len));
}
void send200Http(const String &httpData) {
  httpServer.send(200, String(F("text/html")), httpData );
}
void send500HttpUploadFailed() {
  String httpData = htmlHeader();
  httpData += F("500: couldn't create file");
  httpData += FPSTR(HTTP_END);
  httpServer.send(500, "text/html", httpData );
}

void gencookie() {
  sessioncookie = "";
  for ( uint8_t i = 0; i < 32; i++)
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
  sendLocationHeader(login_url);
  sendCacheControlHeader();
  httpServer.send(301);

  return false;
}


/** Is this an IP? */
boolean isIp(const String &str) {
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
    sendLocationHeader(String(F("http://")) + IPtoString(httpServer.client().localIP()));
    httpServer.send ( 302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    httpServer.client().stop(); // Stop is needed because we sent no content length
    return true;
  }
  return false;
}

void handleNotFound() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the error page.
    return;
  String httpData = htmlHeader();
  httpData += F("404: not found: ");
  httpData += httpServer.uri();
  httpData += FPSTR(HTTP_END);
  sendCacheControlHeader(httpData.length());
  httpServer.send(404, "text/html", httpData);
}

void handleLogin() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;

  String httpData = htmlHeader();

  httpData += F("<div id=\"login\"><form action='/login' method='POST'>Login:");
  httpData += htmlBR;
  httpData += F("<input type='text' name='user' placeholder='User name'></p><p><input type='password' name='pass' placeholder='Password'></p>");
  httpData += htmlBR;
  httpData += F("<button type='submit' name='submit'>login</button></center></form></body></html>");

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
      httpServer.sendHeader("Set-Cookie", "c=" + sessioncookie);
      httpServer.send(301);
      trycount = 0;                                      // With good headers in mind, reset the trycount buffer
      httpLoggedin = true;
      return;
    }

    httpData += "<center>";
    httpData += htmlBR;
    if (trycount != 3 && !lock)
      trycount++;                                        // If system is not locked up the trycount buffer
    if (trycount < 3 && !lock) {                         // We go here if systems isn't locked out, we give user 10 times to make a mistake after we lock down the system, thus making brute force attack almost imposible
      httpData += String(F("<p>Wrong username/password</p>You have "));
      httpData += (3 - trycount);
      httpData += String(F(" tries before system temporarily locks out."));
      logincld = millis();                               // Reset the logincld timer, since we still have available tries
    }

    if (trycount == 3) {                                 // If too many bad tries
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
  sendCacheControlHeader(httpData.length());
  send200Http( httpData );
}

void handleLogout() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;

  //Set 'c=0', it users header, effectively deleting it's header
  httpServer.sendHeader("Set-Cookie", "c=0");
  sendLocationHeader(login_url);
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

  String httpData = htmlHeader();

  httpData += httpSensorData();
  httpData += htmlBR;

  /// send uptime and signal
  int signal = WiFi.RSSI();
  httpData += tableStart;
  httpData += trStart + F("Uptime:") + tdBreak + uptimeString() + trEnd;
  httpData += trStart + F("Signal:") + tdBreak + String(signal) + " dB" + trEnd;
  httpData += trStart + F("MQTT:") + tdBreak;
  if (mqttClient.connected()) {
    httpData += str_tick_green;
  } else {
    httpData += str_cross_red;
  }
  httpData += trEnd;
  httpData += tableEnd;

  sprintf_P( buf, HTTP_META_REFRESH, 3);  // Refresh the page after 3 seconds
  httpData += String(buf);

  httpData += FPSTR(HTTP_END);
  sendCacheControlHeader(httpData.length());
  send200Http( httpData );
}


void handleSystem() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;
  if (!is_authenticated()) {
    return;
  }
  tempign = millis(); //reset the inactivity timer if someone logs in
  char buf[50];

  String httpData = htmlHeader();

  httpData += tableStart;

  httpData += trStart + F("Firmware type:")    + tdBreak + String(fwname)    + trEnd;
  httpData += trStart + F("Firmware version:") + tdBreak + String(FWVERSION) + trEnd;

  httpData += trStart + str_nbsp + tdBreak + trEnd;

  httpData += trStart + F("MAC address:") + tdBreak;
  httpData += String(WiFi.macAddress());
  httpData += trEnd;

  httpData += trStart + str_nbsp + tdBreak + trEnd;

  httpData += trStart + F("Flash size:") + tdBreak;
  httpData += String(ESP.getFlashChipRealSize() / 1048576.0) + " MB";
  httpData += trEnd;

  httpData += trStart + F("Flash size config:") + tdBreak;
  httpData += String(ESP.getFlashChipSize() / 1048576.0) + " MB";
  httpData += trEnd;

  httpData += trStart + F("Program size:") + tdBreak;
  httpData += String(ESP.getSketchSize() / 1024) + " kB";
  httpData += trEnd;

  httpData += trStart + F("Free program size:") + tdBreak;
  httpData += String(ESP.getFreeSketchSpace() / 1024) + " kB";
  httpData += trEnd;

  httpData += trStart + str_nbsp + tdBreak + trEnd;

  httpData += trStart + F("Free memory:") + tdBreak;
  httpData += String(ESP.getFreeHeap() / 1024) + " kB";
  httpData += trEnd;

  httpData += trStart + F("ESP chip ID:") + tdBreak;
  httpData += String(ESP.getChipId());
  httpData += trEnd;

  httpData += trStart + F("Flash chip ID:") + tdBreak;
  httpData += String(ESP.getFlashChipId());
  httpData += trEnd;

  httpData += trStart + str_nbsp + tdBreak + trEnd;

  FSInfo fs_info;
  SPIFFS.info(fs_info);

  httpData += trStart + F("Filesystem size:") + tdBreak;
  httpData += String(fs_info.totalBytes / 1024) + " kB";
  httpData += trEnd;

  httpData += trStart + F("Filesystem free:") + tdBreak;
  httpData += String((fs_info.totalBytes - fs_info.usedBytes) / 1024) + " kB";
  httpData += trEnd;

  httpData += trStart + str_nbsp + tdBreak + trEnd;

  /// send uptime and signal
  httpData += trStart + F("Uptime:") + tdBreak + uptimeString() + trEnd;

  httpData += trStart + str_nbsp + tdBreak + trEnd;

  httpData += trStart + F("SSID:")   + tdBreak + String(ssid) + trEnd;
  httpData += trStart + F("Signal:") + tdBreak + String(WiFi.RSSI()) + " dB" + trEnd;
  httpData += trStart + F("IP:")     + tdBreak + IPtoString(WiFi.localIP()) + trEnd;
  httpData += trStart + str_nbsp + tdBreak + trEnd;
  httpData += tableEnd;

  sprintf_P( buf, HTTP_META_REFRESH, 3);  // Refresh the page after 3 seconds
  httpData += String(buf);
  httpData += FPSTR(HTTP_END);

  sendCacheControlHeader(httpData.length());
  send200Http( httpData );
}


void handleConfig() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;
  if (!is_authenticated()) {
    return;
  }
  tempign = millis(); //reset the inactivity timer if someone logs in

  IPAddress tmpip;
  String httpData = htmlHeader();

  if (httpServer.args()) {

    if (httpServer.hasArg(str_cfg_staticip)) {
      if (httpServer.arg(str_cfg_staticip) == str_true) {
        if (! use_staticip) {
          logString      = str_cfg_changed + str_cfg_staticip + str_from + str_false + str_to + str_true;
          if (!inConfigAP) {
            rebootRequired = true;        // Changing to a static IP requires a reboot
          }
          use_staticip   = true;
          logMessage(app_name_cfg, logString, false);
        }
      } else {
        if (use_staticip) {
          logString      = str_cfg_changed + str_cfg_staticip + str_from + str_true + str_to + str_false;
          if (!inConfigAP) {
            rebootRequired = true;        // Changing to a static IP requires a reboot
          }
          use_staticip   = false;
          logMessage(app_name_cfg, logString, false);
        }
      }
    }

    if (httpServer.hasArg(cfg_ip_address)) {
      tmpip.fromString(httpServer.arg(cfg_ip_address));
      if (local_ip != tmpip) {
        logString = str_cfg_changed + str_cfg_ip_address + str_from + IPtoString(local_ip) + str_to + IPtoString(tmpip);
        if ( (use_staticip) && (!inConfigAP) )
          rebootRequired = true;        // Changing IP requires a reboot
        local_ip = tmpip;
        logMessage(app_name_cfg, logString, false);
      }
    }
    if (httpServer.hasArg(cfg_subnet)) {
      tmpip.fromString(httpServer.arg(cfg_subnet));
      if (subnet != tmpip) {
        logString = str_cfg_changed + str_cfg_subnet + str_from + IPtoString(subnet) + str_to + IPtoString(tmpip);
        if ( (use_staticip) && (!inConfigAP) )
          rebootRequired = true;        // Changing subnet requires a reboot
        subnet = tmpip;
        logMessage(app_name_cfg, logString, false);
      }
    }
    if (httpServer.hasArg(cfg_gateway)) {
      tmpip.fromString(httpServer.arg(cfg_gateway));
      if (gateway != tmpip) {
        logString = str_cfg_changed + str_cfg_gateway + str_from + IPtoString(gateway) + str_to + IPtoString(tmpip);
        if (use_staticip)
          rebootRequired = true;        // Changing gateway IP requires a reboot
        gateway = tmpip;
        logMessage(app_name_cfg, logString, false);
      }
    }
    if (httpServer.hasArg(cfg_dns_server1)) {
      tmpip.fromString(httpServer.arg(cfg_dns_server1));
      if (dns1 != tmpip) {
        logString = str_cfg_changed + str_cfg_dns_server1 + str_from + IPtoString(dns1) + str_to + IPtoString(tmpip);
        if ( (use_staticip) && (!inConfigAP) )
          rebootRequired = true;        // Changing DNS server requires a reboot
        dns1 = tmpip;
        logMessage(app_name_cfg, logString, false);
      }
    }
    if (httpServer.hasArg(cfg_dns_server2)) {
      tmpip.fromString(httpServer.arg(cfg_dns_server2));
      if (dns2 != tmpip) {
        logString = str_cfg_changed + str_cfg_dns_server2 + str_from + IPtoString(dns2) + str_to + IPtoString(tmpip);
        if ( (use_staticip) && (!inConfigAP) )
          rebootRequired = true;        // Changing DNS server requires a reboot
        dns2 = tmpip;
        logMessage(app_name_cfg, logString, false);
      }
    }
    if (httpServer.hasArg(cfg_ntp_server1)) {
      tmpString = String(ntp_server1);
      if (httpServer.arg(cfg_ntp_server1) != tmpString) {
        if (!inConfigAP)
          rebootRequired = true;          // Changing NTP servers requires a reboot
        logString = str_cfg_changed + str_cfg_ntp_server1 + str_from + tmpString + str_to + httpServer.arg(cfg_ntp_server1);
        httpServer.arg(cfg_ntp_server1).toCharArray(ntp_server1, 40);
        logMessage(app_name_cfg, logString, false);
      }
    }
    if (httpServer.hasArg(cfg_ntp_server2)) {
      tmpString = String(ntp_server2);
      if (httpServer.arg(cfg_ntp_server2) != tmpString) {
        logString = str_cfg_changed + str_cfg_ntp_server2 + str_from + tmpString + str_to + httpServer.arg(cfg_ntp_server2);
        if (!inConfigAP)
          rebootRequired = true;          // Changing NTP servers requires a reboot
        httpServer.arg(cfg_ntp_server2).toCharArray(ntp_server2, 40);
        logMessage(app_name_cfg, logString, false);
      }
    }
    if (httpServer.hasArg(cfg_tzoffset)) {
      if (httpServer.arg(cfg_tzoffset).toFloat() != tzoffset) {
        if (!inConfigAP)
          rebootRequired = true;          // Changing Time Zones requires a reboot
        logString = str_cfg_changed + str_cfg_tzoffset + str_from + String(tzoffset) + str_to + httpServer.arg(cfg_tzoffset);
        tzoffset = httpServer.arg(cfg_tzoffset).toFloat();
        logMessage(app_name_cfg, logString, false);
      }
    }

    if (httpServer.hasArg(cfg_mqtt_server)) {
      tmpString = String(mqtt_server);
      if (httpServer.arg(cfg_mqtt_server) != tmpString) {
        logString = str_cfg_changed + str_cfg_mqtt_server + str_from + tmpString + str_to + httpServer.arg(cfg_mqtt_server);
        httpServer.arg(cfg_mqtt_server).toCharArray(mqtt_server, 40);
        if (!inConfigAP) {
          mqttClient.disconnect();        // MQTT will reconnect automatically with the new value
          mqttClient.setServer(mqtt_server, mqtt_port);
        }
        logMessage(app_name_cfg, logString, false);
      }
    }
    if (httpServer.hasArg(cfg_mqtt_port)) {
      if (httpServer.arg(cfg_mqtt_port).toInt() != mqtt_port) {
        logString = str_cfg_changed + str_cfg_mqtt_port + str_from + String(mqtt_port) + str_to + httpServer.arg(cfg_mqtt_port);
        mqtt_port = httpServer.arg(cfg_mqtt_port).toInt();
        if (!inConfigAP) {
          mqttClient.disconnect();        // MQTT will reconnect automatically with the new value
          mqttClient.setServer(mqtt_server, mqtt_port);
        }
        logMessage(app_name_cfg, logString, false);
      }
    }
    if (httpServer.hasArg(cfg_mqtt_topicbase)) {
      tmpString = String(mqtt_topicbase);
      if (httpServer.arg(cfg_mqtt_topicbase) != tmpString) {
        logString = str_cfg_changed + str_cfg_mqtt_topicbase + str_from + tmpString + str_to + httpServer.arg(cfg_mqtt_topicbase);
        if (!inConfigAP)
          mqttClient.disconnect();        // MQTT will reconnect automatically with the new value
        httpServer.arg(cfg_mqtt_topicbase).toCharArray(mqtt_topicbase, 20);
        logMessage(app_name_cfg, logString, false);
      }
    }
    if (httpServer.hasArg(cfg_mqtt_name)) {
      tmpString = String(mqtt_name);
      if (httpServer.arg(cfg_mqtt_name) != tmpString) {
        logString = str_cfg_changed + str_cfg_mqtt_name + str_from + tmpString + str_to + httpServer.arg(cfg_mqtt_name);
        if (!inConfigAP)
          mqttClient.disconnect();        // MQTT will reconnect automatically with the new value
        httpServer.arg(cfg_mqtt_name).toCharArray(mqtt_name, 20);
        logMessage(app_name_cfg, logString, false);
      }
    }
    if (httpServer.hasArg(cfg_mqtt_tls)) {
      if (httpServer.arg(cfg_mqtt_tls) == str_on) {
        if (!mqtt_tls) {
          logString      = str_cfg_changed + str_cfg_mqtt_tls + str_from + str_false + str_to + str_true;
          if (!inConfigAP)
            rebootRequired = true;        // Enabling TLS requires a reboot, as the server certificate verification will fail and force a reboot otherwise
          mqtt_tls       = true;
          logMessage(app_name_cfg, logString, false);
        }
      } else {
        if (mqtt_tls) {
          logString      = str_cfg_changed + str_cfg_mqtt_tls + str_from + str_true + str_to + str_false;
          if (!inConfigAP)
            rebootRequired = true;        // Disabling TLS requires a reboot, as it might not reconnect properly after this.
          mqtt_tls       = false;
          logMessage(app_name_cfg, logString, false);
        }
      }
    } else {
      if (mqtt_tls) {
        logString      = str_cfg_changed + str_cfg_mqtt_tls + str_from + str_true + str_to + str_false;
        if (!inConfigAP)
          rebootRequired = true;        // Disabling TLS requires a reboot, as it might not reconnect properly after this.
        mqtt_tls       = false;
        logMessage(app_name_cfg, logString, false);
      }
    }
    if (httpServer.hasArg(cfg_mqtt_auth)) {
      if (httpServer.arg(cfg_mqtt_auth) == str_on) {
        if (!mqtt_auth) {
          logString = str_cfg_changed + str_cfg_mqtt_auth + str_from + str_false + str_to + str_true;
          mqtt_auth = true;
          if (!inConfigAP)
            mqttClient.disconnect();      // MQTT will reconnect automatically with the new value
          logMessage(app_name_cfg, logString, false);
        }
      } else {
        if (mqtt_auth) {
          logString = str_cfg_changed + str_cfg_mqtt_auth + str_from + str_true + str_to + str_false;
          mqtt_auth = false;
          if (!inConfigAP)
            mqttClient.disconnect();      // MQTT will reconnect automatically with the new value
          logMessage(app_name_cfg, logString, false);
        }
      }
    } else {
      if (mqtt_auth) {
        logString = str_cfg_changed + str_cfg_mqtt_auth + str_from + str_true + str_to + str_false;
        mqtt_auth = false;
        if (!inConfigAP)
          mqttClient.disconnect();        // MQTT will reconnect automatically with the new value
        logMessage(app_name_cfg, logString, false);
      }
    }
    if (httpServer.hasArg(cfg_mqtt_user)) {
      tmpString = String(mqtt_user);
      if (httpServer.arg(cfg_mqtt_user) != tmpString) {
        logString = str_cfg_changed + str_cfg_mqtt_user + str_from + tmpString + str_to + httpServer.arg(cfg_mqtt_user);
        if (!inConfigAP)
          mqttClient.disconnect();        // MQTT will reconnect automatically with the new value
        httpServer.arg(cfg_mqtt_user).toCharArray(mqtt_user, 20);
        logMessage(app_name_cfg, logString, false);
      }
    }
    if (httpServer.hasArg(cfg_mqtt_passwd)) {
      tmpString = String(mqtt_passwd);
      if (httpServer.arg(cfg_mqtt_passwd) != tmpString) {
        logString = str_cfg_changed + str_cfg_mqtt_passwd;
        if (!inConfigAP)
          mqttClient.disconnect();        // MQTT will reconnect automatically with the new value
        httpServer.arg(cfg_mqtt_passwd).toCharArray(mqtt_passwd, 32);
        logMessage(app_name_cfg, logString, false);
      }
    }

    if (httpServer.hasArg(cfg_mqtt_interval)) {
      if (httpServer.arg(cfg_mqtt_interval).toInt() != mqtt_interval) {
        logString = str_cfg_changed + str_cfg_mqtt_interval + str_from + String(mqtt_interval) + str_to + httpServer.arg(cfg_mqtt_interval);
        mqtt_interval = httpServer.arg(cfg_mqtt_interval).toInt();
        logMessage(app_name_cfg, logString, false);
      }
    }

    if (httpServer.hasArg(cfg_mqtt_watchdog)) {
      if (httpServer.arg(cfg_mqtt_watchdog).toInt() != mqtt_watchdog) {
        logString = str_cfg_changed + str_cfg_mqtt_watchdog + str_from + String(mqtt_watchdog) + str_to + httpServer.arg(cfg_mqtt_watchdog);
        mqtt_watchdog = httpServer.arg(cfg_mqtt_watchdog).toInt();
        logMessage(app_name_cfg, logString, false);
      }
      if (mqtt_watchdog <= mqtt_interval) {
        logString = str_cfg_overridden + str_cfg_mqtt_watchdog + str_from + String(mqtt_watchdog) + str_to + String(mqtt_interval + 1);
        mqtt_watchdog = mqtt_interval + 1;
        logMessage(app_name_cfg, logString, false);
      }
      if (mqtt_watchdog < 60) {
        logString = str_cfg_overridden + str_cfg_mqtt_watchdog + str_from + String(mqtt_interval) + str_to + String(60);
        mqtt_watchdog = 60;
        logMessage(app_name_cfg, logString, false);
      }
    }

    if (httpServer.hasArg(cfg_syslog_server)) {
      tmpString = String(syslog_server);
      if (httpServer.arg(cfg_syslog_server) != tmpString) {
        logString = str_cfg_changed + str_cfg_syslog_server + str_from + tmpString + str_to + httpServer.arg(cfg_syslog_server);
        logMessage(app_name_cfg, logString, false);                       // Log this message BOTH before
        httpServer.arg(cfg_syslog_server).toCharArray(syslog_server, 40);
        logMessage(app_name_cfg, logString, false);                       // AND after changing the syslog server
      }
    }
    if (httpServer.hasArg(cfg_use_syslog)) {
      if (httpServer.arg(cfg_use_syslog) == str_on) {
        if (! use_syslog) {
          logString = str_cfg_changed + str_cfg_use_syslog + str_from + str_false + str_to + str_true;
          if (!inConfigAP)
            setupSyslog();      // Syslog changed to enabled
          use_syslog = true;
          logMessage(app_name_cfg, logString, false);
        }
      } else {
        if (use_syslog) {
          logString = str_cfg_changed + str_cfg_use_syslog + str_from + str_true + str_to + str_false;
          logMessage(app_name_cfg, logString, false);   // Log this message BEFORE turning off syslog
          use_syslog = false;
        }
      }
    } else {
      if (use_syslog) {
        logString = str_cfg_changed + str_cfg_use_syslog + str_from + str_true + str_to + str_false;
        use_syslog = false;
        logMessage(app_name_cfg, logString, false);   // Log this message BEFORE turning off syslog
      }
    }

    // Check for any custom sensor config
    httpSensorConfig();

    saveConfig();
  }

  httpData += F("<form action='#' method='POST'>");
  httpData += tableStart;

  httpData += thStart + F("Network:") + thBreak + thEnd;
  httpData += trStart + "IP:"                 + tdBreak;
  httpData += htmlRadio(str_cfg_staticip, str_true,          use_staticip)     + str_static;
  httpData += htmlRadio(str_cfg_staticip, str_false,         (! use_staticip)) + str_dhcp;
  httpData += trEnd;
  httpData += trStart + str_cfg_ip_address    + tdBreak + htmlInput(str_text,     cfg_ip_address,    IPtoString(local_ip))      + trEnd;
  httpData += trStart + str_cfg_subnet        + tdBreak + htmlInput(str_text,     cfg_subnet,        IPtoString(subnet))  + trEnd;
  httpData += trStart + str_cfg_gateway       + tdBreak + htmlInput(str_text,     cfg_gateway,       IPtoString(gateway)) + trEnd;
  httpData += trStart + str_cfg_dns_server1   + tdBreak + htmlInput(str_text,     cfg_dns_server1,   IPtoString(dns1));
  tmpString = IPtoString(dns1);
  if ( use_staticip ) {
    if ( WiFi.hostByName(tmpString.c_str(), tmpip) ) {
      httpData += str_tick_green;
    } else {
      httpData += str_cross_red;
    }
  }
  httpData += trEnd;
  httpData += trStart + str_cfg_dns_server2   + tdBreak + htmlInput(str_text,     cfg_dns_server2,   IPtoString(dns2));
  tmpString = IPtoString(dns2);
  if ( use_staticip ) {
    if ( WiFi.hostByName(tmpString.c_str(), tmpip) ) {
      httpData += str_tick_green;
    } else {
      httpData += str_cross_red;
    }
  }
  httpData += trEnd;

  httpData += trStart + str_cfg_ntp_server1   + tdBreak + htmlInput(str_text,     cfg_ntp_server1,   ntp_server1);
  if ( WiFi.hostByName(ntp_server1, tmpip) ) {
    httpData += str_tick_green;
  } else {
    httpData += str_cross_red;
  }
  httpData += trEnd;
  httpData += trStart + str_cfg_ntp_server2   + tdBreak + htmlInput(str_text,     cfg_ntp_server2,   ntp_server2);
  if ( WiFi.hostByName(ntp_server2, tmpip) ) {
    httpData += str_tick_green;
  } else {
    httpData += str_cross_red;
  }
  httpData += trEnd;
  httpData += trStart + str_cfg_tzoffset      + tdBreak + htmlInput(str_text,     cfg_tzoffset,     String(tzoffset)) + F("(in hours, e.g. UTC+10 is 10, UTC-5 is -5)") + trEnd;


  httpData += thStart + F("MQTT:") + thBreak + thEnd;
  httpData += trStart + "Server:"             + tdBreak + htmlInput(str_text,     cfg_mqtt_server,   mqtt_server);
  if (mqttClient.connected()) {
    httpData += str_tick_green;
  } else {
    httpData += str_cross_red;
  }
  httpData += trEnd;
  httpData += trStart + "Port:"               + tdBreak + htmlInput(str_text,     cfg_mqtt_port,      String(mqtt_port))     + trEnd;
  httpData += trStart + "Topic base:"         + tdBreak + htmlInput(str_text,     cfg_mqtt_topicbase, mqtt_topicbase)        + trEnd;
  httpData += trStart + "Name:"               + tdBreak + htmlInput(str_text,     cfg_mqtt_name,      mqtt_name)             + trEnd;
  httpData += trStart + "Use TLS:"            + tdBreak + htmlCheckbox(           cfg_mqtt_tls,       mqtt_tls)              + trEnd;
  httpData += trStart + "Use authentication:" + tdBreak + htmlCheckbox(           cfg_mqtt_auth,      mqtt_auth)             + trEnd;
  httpData += trStart + "Username:"           + tdBreak + htmlInput(str_text,     cfg_mqtt_user,      mqtt_user)             + trEnd;
  httpData += trStart + "Password:"           + tdBreak + htmlInput(str_text,     cfg_mqtt_passwd,    mqtt_passwd)           + trEnd;
  httpData += trStart + "Data interval (s):"  + tdBreak + htmlInput(str_text,     cfg_mqtt_interval,  String(mqtt_interval)) + trEnd;
  httpData += trStart + "MQTT watchdog (s):"  + tdBreak + htmlInput(str_text,     cfg_mqtt_watchdog,  String(mqtt_watchdog)) + trEnd;

  httpData += thStart + F("Syslog:") + thBreak + thEnd;
  httpData += trStart + str_cfg_use_syslog    + tdBreak + htmlCheckbox(           cfg_use_syslog,    use_syslog)    + trEnd;
  httpData += trStart + str_cfg_syslog_server + tdBreak + htmlInput(str_text,     cfg_syslog_server, syslog_server);
  if ( use_syslog && WiFi.hostByName(syslog_server, tmpip) ) {
    httpData += str_tick_green;
  } else {
    httpData += str_cross_red;
  }
  httpData += trEnd;

  httpData += thStart + "Sensors:" + thBreak + thEnd;

  httpData += httpSensorSetup();

  httpData += tableEnd;
  httpData += FPSTR(HTTP_FORM_END);

  httpData += FPSTR(HTTP_END);
  sendCacheControlHeader(httpData.length());
  send200Http( httpData );
}


void handlePassword() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;
  if (!is_authenticated()) {
    return;
  }
  tempign = millis(); //reset the inactivity timer if someone logs in

  String httpData = htmlHeader();

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
          logMessage(app_name_sys, logString, false);
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
            logMessage(app_name_sys, logString, false);
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

  httpData += trStart + "Username:"           + tdBreak + htmlInput(str_text,     "newUser",  httpUser) + trEnd;
  httpData += trStart + "Current password:"   + tdBreak + htmlInput(str_password, "currentPasswd", "")  + trEnd;
  httpData += trStart + str_nbsp + tdBreak + trEnd;
  httpData += trStart + "New password:"       + tdBreak + htmlInput(str_password, "newPasswd1", "")     + trEnd;
  httpData += trStart + "Repeat new:"         + tdBreak + htmlInput(str_password, "newPasswd2", "")     + trEnd;

  httpData += trEnd;
  httpData += tableEnd;

  httpData += FPSTR(HTTP_FORM_END);

  httpData += FPSTR(HTTP_END);
  sendCacheControlHeader(httpData.length());
  send200Http( httpData );
}



/** Wifi config page handler */
void handleWifi() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;
  if (!is_authenticated()) {
    return;
  }
  tempign = millis(); //reset the inactivity timer if someone logs in

  String httpData = htmlHeader();

  if (! inConfigAP) {
    httpData += str_h2_red;
    httpData.replace("{v}", F("Warning"));
    httpData += (String(F("<p>You are connected through the wifi network: ")) + ssid + htmlBR);
    httpData += (String(F("<b>If you change WiFi networks you may not be able to reconnect to the device.</b></p>")));
  }

  int signal = WiFi.RSSI();
  ssid = WiFi.SSID();
  if (ssid != "") {
    httpData += tableStart;
    httpData += thStart + F("Current config:") + thBreak + thEnd;
    httpData += trStart + F("SSID:")   + tdBreak + String(ssid) + trEnd;
    if (! inConfigAP) {
      httpData += trStart + F("Signal:") + tdBreak + String(signal) + " dB" + trEnd;
      httpData += trStart + F("IP:")     + tdBreak + IPtoString(WiFi.localIP()) + trEnd;
    }
    httpData += trEnd;
    httpData += tableEnd;

    httpData += htmlBR;
  }
  if (inConfigAP) {
    httpData += String(F("After connecting the device will be available on http://")) + String(host_name) + F(".local/") + htmlBR;
  }

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
      yield();
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
  sendCacheControlHeader(httpData.length());
  send200Http( httpData );
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
  String httpData;

  // Store new credentials here for checking later
  _ssid = httpServer.arg("s");
  _psk  = httpServer.arg("p");

  if (_ssid != "" && _psk != "") {
    httpData = FPSTR(HTTP_HEAD);
    httpData.replace("{v}", F("Credentials Saved"));
    httpData += FPSTR(HTTP_SCRIPT);
    httpData += FPSTR(HTTP_STYLE);
    httpData += FPSTR(HTTP_HEAD_END);
    httpData.replace("{t}", String(host_name));
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
  sendCacheControlHeader(httpData.length());
  send200Http( httpData );
}


void handleReboot() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;
  if (!is_authenticated()) {
    return;
  }
  tempign = millis(); //reset the inactivity timer if someone logs in
  char buf[50];

  String httpData = htmlHeader();
  sprintf_P( buf, HTTP_META_REFRESH, 10);  // Refresh the page after 10 seconds
  httpData += String(buf);
  httpData += str_rebooting;
  httpData += FPSTR(HTTP_END);

  sendCacheControlHeader(httpData.length());
  send200Http( httpData );

  delay(100);
  ESP.restart();
  delay(5000);
}


//////////////////////////////////// upload pages ////////////////////////////////////
void handleFirmware() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;
  if (!is_authenticated()) {
    return;
  }
  tempign = millis();  // reset the inactivity timer if someone logs in

  String httpData = htmlHeader();

  httpData += String(F("Select a firmware file to upload.")) + htmlBR + F("Be certain to make sure it's compiled with the same SPIFFS size and config pin or the next boot will fail.") + htmlBR;

  httpData += F("<table style='border:2px solid red;margin:1em;'>");
  httpData += trStart + F("Flash size config:") + tdBreak;
  httpData += String(ESP.getFlashChipSize() / 1048576.0) + " MB";
  httpData += trEnd;
  FSInfo fs_info;
  SPIFFS.info(fs_info);
  httpData += trStart + F("SPIFFS size:") + tdBreak;
  httpData += String(fs_info.totalBytes / 1024) + " kB";
  httpData += trEnd;
  httpData += trStart + F("Config pin:") + tdBreak;
  httpData += String(CONFIG_PIN);
  httpData += trEnd;
  httpData += trStart + F("Firmware type:")    + tdBreak + String(fwname)    + trEnd;
  httpData += tableEnd;

  httpData += F("<form action='#' method='POST' enctype='multipart/form-data'>");
  httpData += F("<input type='file' name='firmware'>");
  httpData += htmlBR;
  httpData += F("<button type='submit'>Update</button></form>");

  httpData += FPSTR(HTTP_END);
  sendCacheControlHeader(httpData.length());
  send200Http( httpData );
}

void handleFirmwareUploadComplete() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;
  if (!is_authenticated()) {
    return;
  }
  tempign = millis();  // reset the inactivity timer if someone logs in
  char buf[50];

  String httpData = "</p>";   // Finishes the progress bar

  if (Update.hasError() || firmwareUpdateError != "") {
    httpData += str_h2_red;
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
    httpData += String(uploadedFileSize) + F(" bytes uploaded.") + htmlBR + str_rebooting + F("</p></body></html>");
    httpServer.client().setNoDelay(true);
    httpServer.sendContent(httpData);
    delay(100);
    httpServer.client().stop();
    delay(100);
    ESP.restart();
  }
}

void handleFirmwareUpload() {
  if (captivePortal())  // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;
  if (!is_authenticated()) {
    return;
  }
  tempign = millis();   // reset the inactivity timer if someone logs in
  String httpData;

  HTTPUpload& upload = httpServer.upload();

  if (upload.status == UPLOAD_FILE_START) {
    firmwareUpdateError = "";
    uploadedFileSize    = 0;
    Serial.setDebugOutput(true);
    logString = str_firmware_update + upload.filename;
    logMessage(app_name_sys, logString, false);
    delay(10);          // Give time for the syslog packet to be sent by UDP
    WiFiUDP::stopAll();
    mqttClient.disconnect();        // Disconnect from MQTT to save some RAM during firmware update

    httpServer.client().setNoDelay(true);
    httpServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
    send200Http("");    // Empty content inhibits Content-length header so we have to close the socket ourselves.

    httpData = htmlHeader();
    httpData += F("<p style='background-color:#0A0;width:fit-content;font-size:5px;'>");

    httpServer.sendContent(httpData);
    delay(100);         // Allow a moment for the current http data to be sent

    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if (!Update.begin(maxSketchSpace)) {                  // start with max available size
      Update.printError(Serial);
      StreamString str;
      Update.printError(str);
      firmwareUpdateError = str.c_str();
    }
  } else if (upload.status == UPLOAD_FILE_WRITE && !firmwareUpdateError.length()) {
    Serial.print(str_dot);
    httpServer.sendContent("_");

    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
      StreamString str;
      Update.printError(str);
      firmwareUpdateError = str.c_str();
      logString = str_firmware_update + firmwareUpdateError;
      logMessage(app_name_sys, logString, false);
    }
  } else if (upload.status == UPLOAD_FILE_END && !firmwareUpdateError.length()) {
    if (Update.end(true)) {                               // true to set the size to the current progress
      uploadedFileSize = upload.totalSize;
      if (upload.totalSize != 0 ) {
        Serial.printf_P( PSTR("\nUpdate Success: %u bytes\nReboot required...\n"), upload.totalSize);
      } else {
        firmwareUpdateError = F("no data uploaded.");
        logString = str_firmware_update + firmwareUpdateError;
        logMessage(app_name_sys, logString, false);
      }
    } else {
      Update.printError(Serial);
      StreamString str;
      Update.printError(str);
      firmwareUpdateError = str.c_str();
      logString = str_firmware_update + firmwareUpdateError;
      logMessage(app_name_sys, logString, false);
    }
    Serial.setDebugOutput(false);
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.end();
    firmwareUpdateError = F("Update was aborted");
    logString = str_firmware_update + firmwareUpdateError;
    logMessage(app_name_sys, logString, false);
  }
  yield();
}


/** Handle the CA certificate upload page */
void handleCACert() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;
  if (!is_authenticated()) {
    return;
  }
  tempign = millis(); //reset the inactivity timer if someone logs in

  String httpData = htmlHeader();

  httpData += F("<p>This CA certificate is for the secure TLS MQTT connection.</p>");

  if ( SPIFFS.exists(CAcertfilename) ) {
    File certfile = SPIFFS.open(CAcertfilename, "r");
    httpData += F("<div style='border:2px solid red;margin:1em;'>");
    httpData += str_h2_red;
    httpData.replace("{v}", F("Existing certificate"));
    httpData += F("There is an existing CA certificate with a size of ");
    httpData += certfile.size();
    httpData += F(" bytes</div>");
  }

  httpData += F("Select a file to upload with the CA certificate in DER format.");
  httpData += htmlBR;
  httpData += F("A certificate can be converted to DER format with openssl with the following command edited as appropriate:");
  httpData += F("<pre>openssl x509 -in CACert.pem -out CACert.der -outform DER</pre>");
  httpData += F("<form action='#' method='POST' enctype='multipart/form-data'>");
  httpData += F("<input type='file' name='cacert'>");
  httpData += htmlBR;
  httpData += F("<button type='submit'>Upload</button></form>");

  httpData += FPSTR(HTTP_END);
  sendCacheControlHeader(httpData.length());
  send200Http( httpData );
}

/* Handle a file being uploaded */
void handleCACertUpload() {
  if (captivePortal()) // If captive portal and not connected by IP address redirect instead of displaying the page.
    return;
  if (!is_authenticated()) {
    return;
  }
  tempign = millis(); //reset the inactivity timer if someone logs in

  rebootRequired = true;        // Making use of a new CA certificate requires a reboot

  HTTPUpload& upload = httpServer.upload();

  if (upload.status == UPLOAD_FILE_START) {
    if ( SPIFFS.exists(CAcertfilename) )
      SPIFFS.remove(CAcertfilename);
    fsUploadFile = SPIFFS.open(CAcertfilename, "w");      // Save the file as cacert.der no matter the supplied filename
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if (upload.status == UPLOAD_FILE_END) {
    logString = F("CA certificate upload ");
    if (fsUploadFile) {                                   // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      sendLocationHeader(http_page_urls[5]);              // Redirect the client to the "/cacert" page
      httpServer.send(303);
      logString += str_succeeded;
    } else {
      send500HttpUploadFailed();
      logString += str_failed;
    }
    logMessage(app_name_http, logString, false);
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

  String httpData = htmlHeader();

  httpData += str_h2_red;
  httpData.replace("{v}", F("Currently unused!"));
  httpData += F("<p>This certificate and key is for the secure HTTPS interface.");
  httpData += htmlBR;
  httpData += F("It is not needed for MQTT but is a needed for a secure connection to this device's web interface.</p>");

  httpData += F("The files need to be uploaded in DER format.");
  httpData += htmlBR;
  httpData += F("A certificate can be converted to DER format with openssl with the following command edited as appropriate:");
  httpData += F("<pre>openssl x509 -in filename.pem -out filename.pem.der -outform DER</pre>");
  httpData += F("and the key can be converted with:");
  httpData += F("<pre>openssl rsa -in filename.key -out filename.key.der -outform DER</pre>");

  httpData += tableStart;
  httpData += trStart;

  if ( SPIFFS.exists(HTTPScertfilename) ) {
    File certfile = SPIFFS.open(HTTPScertfilename, "r");
    httpData += str_h2_red;
    httpData.replace("{v}", F("Existing certificate"));
    httpData += F("There is an existing HTTPS certificate with a size of ");
    httpData += certfile.size();
    httpData += F(" bytes<hr>");
  }
  httpData += F("<form action='/httpscert' method='POST' enctype='multipart/form-data'>");
  httpData += F("<input type='file' name='httpscert'>");
  httpData += htmlBR;
  httpData += F("<button type='submit'>Upload certificate</button></form>");
  httpData += tdBreak;

  if ( SPIFFS.exists(HTTPSkeyfilename) ) {
    File certfile = SPIFFS.open(HTTPSkeyfilename, "r");
    httpData += str_h2_red;
    httpData.replace("{v}", F("Existing key"));
    httpData += F("There is an existing HTTPS key with a size of ");
    httpData += certfile.size();
    httpData += F(" bytes<hr>");
  }
  httpData += F("<form action='/httpskey' method='POST' enctype='multipart/form-data'>");
  httpData += F("<input type='file' name='httpskey'>");
  httpData += htmlBR;
  httpData += F("<button type='submit'>Upload key</button></form>");
  httpData += trEnd;
  httpData += tableEnd;

  httpData += FPSTR(HTTP_END);
  sendCacheControlHeader(httpData.length());
  send200Http( httpData );
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
    logString = F("HTTPS certificate upload ");
    if (fsUploadFile) {                                   // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      sendLocationHeader(http_page_urls[6]);              // Redirect the client to the "/https" page
      httpServer.send(303);
      logString += str_succeeded;
    } else {
      send500HttpUploadFailed();
      logString += str_failed;
    }
    logMessage(app_name_http, logString, false);
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
    logString = F("HTTPS key upload ");
    if (fsUploadFile) {                                   // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      sendLocationHeader(http_page_urls[6]);              // Redirect the client to the "/https" page
      httpServer.send(303);
      logString += str_succeeded;
    } else {
      send500HttpUploadFailed();
      logString += str_failed;
    }
    logMessage(app_name_http, logString, false);
  }
}

String uptimeString() {
  unsigned long uptime = millis();
  String uptimeStr;

  uptime = uptime / 1000;

  uint16_t uptime_day = uptime / 86400;
  uptime = uptime % 86400;
  uint16_t uptime_hour = uptime / 3600;
  uptime = uptime % 3600;
  uint16_t uptime_minute = uptime / 60;
  uptime = uptime % 60;
  uint16_t uptime_second = uptime;
  bool uptime_comma = false;

  if (uptime_day) {
    uptimeStr += String(uptime_day) + F(" days");
    uptime_comma = true;
  }
  if (uptime_hour) {
    if (uptime_comma)
      uptimeStr += str_comma;
    uptimeStr += String(uptime_hour) + F(" hours");
    uptime_comma = true;
  }
  if (uptime_minute) {
    if (uptime_comma)
      uptimeStr += str_comma;
    uptimeStr += String(uptime_minute) + F(" minutes");
    uptime_comma = true;
  }
  if (uptime_second) {
    if (uptime_comma)
      uptimeStr += str_comma;
    uptimeStr += String(uptime_second) + F(" seconds");
  }

  return uptimeStr;
}
