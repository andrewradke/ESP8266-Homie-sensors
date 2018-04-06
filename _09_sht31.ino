#if FWTYPE == 9      // esp8266-sht31

void sensorSetup() {
  sht31Status = sht31.begin(0x44);   // Set to 0x45 for alternate i2c addr
  logString = "SHT-31 initialised";
  mqttLog(app_name_sensors, logString);
  // The library always returns true for begin() so it's not much point checking the returned value
  logString = "SHT-31 ";
  uint16_t sht31State = sht31.readStatus();
  if (sht31State == 65535) {          // returns 65535 if no sensor (no I2C device at all?) found
    logString   = logString + "NOT ";
    sht31Error  = true;
    sht31Status = false;
  }
  logString = logString + "found";
  mqttLog(app_name_sensors, logString);
/*
  // The library always returns 0x8010 so this is pretty useless too
  logString = "SHT-31 status: 0x" + String(sht31State, HEX);
  mqttLog(app_name_sensors, logString);
*/
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
  temperature = 1000;
  humidity    = 1000;
  dewpoint    = 1000;

  // Start SHT-31
  if (! sht31Status) {
    logString = "Retrying SHT-31";
    mqttLog(app_name_sensors, logString);
    sht31Status  = sht31.begin();
    uint16_t sht31State = sht31.readStatus();
    if (sht31State != 65535) {          // returns 65535 if no sensor (no I2C device at all?) found
      logString = "SHT-31 found";
      sht31Error = false;
      sht31Status = true;
      mqttLog(app_name_sensors, logString);
    } else {
      sht31Status = false;
    }
  } else {

    temperature1 = sht31.readTemperature();
    humidity1    = sht31.readHumidity();


    // Start temperature
    // The lowest recorded temperature is -88C and the highest is 58C so if it's outside this then it's a false reading
    if ( temperature1 > -100 && temperature1 < 65) {

      temperature = temperature1;

      if (sht31ErrorT) {
        if (sht31ErrorT >= error_count_log) {
          logString = "SHT-31 temperature recovered: " + String(temperature1) + "C";
          mqttLog(app_name_sensors, logString);
        }
        sht31ErrorT = 0;
      }
    } else {  // Bad temperature reading
      sht31ErrorT++;
      if (sht31ErrorT == error_count_log) {
        logString = "SHT-31 temperature error: " + String(temperature1) + "C";
        mqttLog(app_name_sensors, logString);
      }
    }
    // End temperature

    // Start humidity
    // 100% humidity means the humidity sensor has gotten wet and is no longer useful
    if ( humidity1 > 0 && humidity1 < 100) {

      humidity = humidity1;

      if (sht31ErrorH) {
        if (sht31ErrorH >= error_count_log) {
          logString = "SHT-31 humidity recovered: " + String(humidity1) + "%";
          mqttLog(app_name_sensors, logString);
        }
        sht31ErrorH = 0;
      }

    } else {  // Bad humidity reading
      sht31ErrorH++;
      if (sht31ErrorH == error_count_log) {
        logString = "SHT-31 humidity error: " + String(humidity1) + "%";
        mqttLog(app_name_sensors, logString);
      }
    }
    // End humidity

  }
  // End SHT-31

  if (temperature != 1000) {
    mqttSend(String("temperature/celsius"),   String(temperature), false);
  } else {
    mqttSend(String("temperature/celsius"),   strNaN,        false);
  }

  if (humidity != 1000) {
    mqttSend(String("humidity/percentage"),   String(humidity), false);
  } else {
    mqttSend(String("humidity/percentage"),   strNaN,        false);
  }

  if ( (temperature != 1000) && (humidity != 1000) ) {
    dewpoint = 5351/(5351/(temperature + 273.15) - log(humidity/100)) - 273.15;
    mqttSend(String("dewpoint/celsius"),      String(dewpoint), false);
  } else {
    mqttSend(String("dewpoint/celsius"),      strNaN,        false);
  }
}

String httpSensorData() {
  String httpData = tableStart;

  httpData += trStart + "Temperature:" + tdBreak;
  if (temperature != 1000) {
    httpData += String(temperature) + " C";
  } else {
    httpData += "-";
  }
  httpData += trEnd;

  httpData += trStart + "Humidity:" + tdBreak;
  if (humidity != 1000) {
    httpData += String(humidity) + " %";
  } else {
    httpData += "-";
  }
  httpData += trEnd;

  httpData += trStart + "Dew point:" + tdBreak;
  if (dewpoint != 1000) {
    httpData += String(dewpoint) + " C";
  } else {
    httpData += "-";
  }
  httpData += trEnd;

  httpData += tableEnd;
  return httpData;
}

String httpSensorSetup() {
  String httpData;
  httpData += trStart + "No config items." + tdBreak + trEnd;
  return httpData;
}

String httpSensorConfig() {
}

#endif
