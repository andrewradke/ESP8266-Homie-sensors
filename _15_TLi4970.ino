#if FWTYPE == 15      // esp8266-tli4970

const char str_acLoad[]  = "acLoad";

void sensorSetup() {
  Tli4970CurrentSensor.begin(SPI, PIN_TLI_CS, PIN_TLI_OCD, PIN_TLI_DIO);
}


void printSensorConfig(const String &cfgStr) {
  mqttSend(String(cfgStr + str_acLoad), String(acLoad), true);  
}

void sensorImportJSON(JsonObject& json) {
  if (json[str_acLoad].is<bool>()) {
    acLoad = json[str_acLoad];
  }
}

void sensorExportJSON(JsonObject& json) {
  json[str_acLoad] = acLoad;
}

bool sensorUpdateConfig(const String &key, const String &value) {
  if ( key == str_acLoad ) {
    if ( value.equalsIgnoreCase("true") or value.equalsIgnoreCase("yes") or value == "1" ) {
      acLoad = true;
    } else if ( value.equalsIgnoreCase("false") or value.equalsIgnoreCase("no") or value == "0" ) {
      acLoad = false;
    } else {
      logString = String(F("Unrecognised value for acLoad: ")) + value;
      mqttLog(app_name_sensors, logString);      
    }
  } else {
    return false;
  }
  return true;
}

bool sensorRunAction(const String &key, const String &value) {
  return false;
}

void calcData() {
  if ( currentTime > (sLoopTime) ) {              // Check current every ms
    sLoopTime = currentTime;                      // Updates sLoopTime
    if (Tli4970CurrentSensor.readOut()) {
      tli4970Errors++;
      if (tli4970Errors == error_count_log) {
        logString = "Tli4970 error";
        mqttLog(app_name_sensors, logString);
      }
  
    } else {
      float thisCurrent = Tli4970CurrentSensor.getCurrent();
      if ( acLoad && thisCurrent < 0 )
        thisCurrent *= -1;
      current += thisCurrent;
      readings++;
  
      if (tli4970Errors) {
        if (tli4970Errors >= error_count_log) {
          logString = "Tli4970 recovered";
          mqttLog(app_name_sensors, logString);
        }
        tli4970Errors = 0;
      }
    }
  }
#ifdef USESSD1306
  if ( millis() < (prevDisplay + (displaySleep * 1000) ) ) {  // if the display should still be awake
    if ( currentTime >= (dLoopTime + 1000) ) {
      dLoopTime = currentTime;                      // Updates dLoopTime for OLED redraw
      display.clearDisplay();
      char text[7];
      float amps = current/readings;
      byte places = 1;
      if (amps < 1)
        places = 3;
      else if (amps < 10)
        places = 2;
      dtostrf(amps, 6, places, text);
      display.setCursor(0, 16);
      display.setTextSize(3);
      display.print(text);
      display.setCursor(115, 23);
      display.setTextSize(2);
      display.print("A");  
      display.display();
    }
  }
#endif
}

void sendData() {
  char text[7];
  float amps = current/readings;
  byte places = 1;
//  current /= readings;      // Return the average of the current readings since the last time data was sent
#ifdef DEBUG
  Serial.print("readings: ");
  Serial.print(readings);
  Serial.print(", current: ");
  Serial.println(current);
#endif
  if (amps < 1)
    places = 3;
  else if (amps < 10)
    places = 2;
  dtostrf(amps, 6, places, text);
  mqttSend(String("current/amps"),   String(text), false);
  current  = 0;
  readings = 0;
}

String httpSensorData() {
  String httpData = tableStart;
  httpData += trStart + "Current:" + tdBreak;
  httpData += String(current/readings) + " amps";
  httpData += trEnd;
  httpData += tableEnd;
  return httpData;
}

String httpSensorSetup() {
  String httpData;
  httpData += trStart + F("Load type:")                 + tdBreak;
  httpData += htmlRadio(str_acLoad, str_false,         (! acLoad)) + "DC";
  httpData += htmlRadio(str_acLoad, str_true,          acLoad)     + "AC";
  httpData += trEnd;
  return httpData;
}

String httpSensorConfig() {
  if (httpServer.hasArg(str_acLoad)) {
    if (httpServer.arg(str_acLoad) == str_true) {
      acLoad = true;
    } else {
      acLoad = false;
    }
  }
}

void sensorMqttCallback(char* topic, byte* payload, unsigned int length) {}
void sensorMqttSubs() {}

#endif
