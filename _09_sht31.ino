#if FWTYPE == 9      // esp8266-sht31


void sensorSetup() {
  app_name = "Sensors";
  sht31Status = sht31.begin(0x44);   // Set to 0x45 for alternate i2c addr
  // The library always returns true for begin() and therefore checking the status below is required to confirm the SHT-31 is working.
/*
  logString = "SHT-31 ";
  if (! sht31Status) {
    logString = logString + " NOT";
    sht31Error = true;
  }
  logString = logString + "found";
  mqttLog(logString);
*/
  uint16_t sht31State = sht31.readStatus();
  logString = "SHT-31 status: 0x" + String(sht31State, HEX);
  mqttLog(logString);
}

void  printSensorConfig(String cfgStr) {
}

void sensorImportJSON(JsonObject& json) {
}

void sensorExportJSON(JsonObject& json) {
}

bool sensorUpdateConfig(String key, String value) {
  return false;
}

bool sensorRunAction(String key, String value) {
  return false;
}


void calcData() {
}

void sendData() {
  app_name = "Sensors";
  // Start SHT-31
  if (! sht31Status) {
    Serial.println("Retrying SHT-31");
    sht31Status  = sht31.begin();
    if (sht31Status) {
      logString = "SHT-31 found";
      sht31Error = true;
      mqttLog(logString);
    }

  } else {

    temperature  = sht31.readTemperature();
    humidity     = sht31.readHumidity();


    // Start temperature
    // The lowest recorded temperature is -88C and the highest is 58C so if it's outside this then it's a false reading
    if ( temperature > -100 && temperature < 65) {

      mqttSend(String("temperature/celsius"),   String(temperature), false);

      if (sht31ErrorT) {
        if (sht31ErrorT >= error_count_log) {
          logString = "SHT-31 temperature recovered: " + String(temperature) + "C";
          mqttLog(logString);
        }
        sht31ErrorT = 0;
      }
    } else {  // Bad temperature reading
      sht31ErrorT++;
      if (sht31ErrorT == error_count_log) {
        logString = "SHT-31 temperature error: " + String(temperature) + "C";
        mqttLog(logString);
      }
      mqttSend(String("temperature/celsius"),   String("nan"), false);
    }
    // End temperature

    // Start humidity
    // 100% humidity means the humidity sensor has gotten wet and is no longer useful
    if ( humidity > 0 && humidity < 100) {

      mqttSend(String("humidity/percentage"),   String(humidity), false);

      if (sht31ErrorH) {
        if (sht31ErrorH >= error_count_log) {
          logString = "SHT-31 humidity recovered: " + String(humidity) + "%";
          mqttLog(logString);
        }
        sht31ErrorH = 0;
      }

    } else {  // Bad humidity reading
      sht31ErrorH++;
      if (sht31ErrorH == error_count_log) {
        logString = "SHT-31 humidity error: " + String(humidity) + "%";
        mqttLog(logString);
      }
      mqttSend(String("humidity/percentage"),   String("nan"), false);
    }
    // End humidity

  }
  // End SHT-31
}

#endif
