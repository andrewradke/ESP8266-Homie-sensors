#if FWTYPE == 10      // esp8266-air

void sensorSetup() {
  bme280Status = bme.begin(0x76);
  logString = "BME280 ";
  if (bme280Status) {
    // The following are the recommended values from shapter 3.5 of the BME280 datasheet with
    // the expeception that the sampling rate is governed by the mqtt_interval value 
    // forced mode, 1x temperature / 1x humidity / 1x pressure oversampling, filter off
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1, // temperature
                    Adafruit_BME280::SAMPLING_X1, // pressure
                    Adafruit_BME280::SAMPLING_X1, // humidity
                    Adafruit_BME280::FILTER_OFF   );
  } else {
    logString = logString + "NOT ";
  }
  logString = logString + "found";
  mqttLog(app_name_sensors, logString);

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

void  printSensorConfig(const String &cfgStr) {
  mqttSend(String(cfgStr + "elevation"), String(elevation), true);
}

void sensorImportJSON(JsonObject& json) {
  if (json["elevation"].is<float>()) {
    elevation = json["elevation"];
  }
}

void sensorExportJSON(JsonObject& json) {
  json["elevation"] = elevation;
}

bool sensorUpdateConfig(const String &key, const String &value) {
  if ( key == "elevation" ) {
    elevation = value.toFloat();
  } else {
    return false;
  }
  return true;
}

bool sensorRunAction(const String &key, const String &value) {
  return false;
}

void calcData() {}

void sendData() {
  temperature = 1000;
  humidity    = 1000;
  dewpoint    = 1000;
  pressure    = 0;
  sealevel    = 0;

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
    // 100% humidity seems to be acceptable on a SHT-31
    if ( humidity1 > 0 && humidity1 <= 100) {

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


  // Start BME280
  if (! bme280Status) {
    logString = "Retrying BME280";
    mqttLog(app_name_sensors, logString);

    bme280Status = bme.begin();
    if (bme280Status) {
      // The following are the recommended values from shapter 3.5 of the BME280 datasheet with
      // the expeception that the sampling rate is governed by the mqtt_interval value 
      // forced mode, 1x temperature / 1x humidity / 1x pressure oversampling, filter off
      bme.setSampling(Adafruit_BME280::MODE_FORCED,
                      Adafruit_BME280::SAMPLING_X1, // temperature
                      Adafruit_BME280::SAMPLING_X1, // pressure
                      Adafruit_BME280::SAMPLING_X1, // humidity
                      Adafruit_BME280::FILTER_OFF   );
      logString = "BME280 found";
      mqttLog(app_name_sensors, logString);
    }

  } else {
    // Only needed in forced mode! In normal mode, you can remove the next line. Normal mode can result in higher temperatures due to the BME280 getting heating up from continuous readings.
    bme.takeForcedMeasurement(); // has no effect in normal mode

    temperature2 = bme.readTemperature();
//    humidity2    = bme.readHumidity();
    humidity2    = 255;    // Permanently treat BME280 humidity as bad
    pressure2    = bme.readPressure() / 100.0;
//  The following equation is from https://www.sandhurstweather.org.uk/barometric.pdf
    sealevel2    = pressure2 / exp(-elevation/((temperature2 + 273.15)*29.263));

    // Start temperature
    // The lowest recorded temperature is -88C and the highest is 58C so if it's outside this then it's a false reading
    if ( temperature2 > -100 && temperature2 < 65) {

      if (sht31ErrorT) {  // If the SHT-31 has a problem report this temperature otherwise let it report it's temperature
        temperature = temperature2;
      }

      if (bme280ErrorT) {
        if (bme280ErrorT >= error_count_log) {
          logString = "BME280 temperature recovered: " + String(temperature2) + "C";
          mqttLog(app_name_sensors, logString);
        }
        bme280ErrorT = 0;
      }
    } else {  // Bad temperature reading
      bme280ErrorT++;
      if (bme280ErrorT == error_count_log) {
        logString = "BME280 temperature error: " + String(temperature2) + "C";
        mqttLog(app_name_sensors, logString);
      }
    }
    // End temperature

    // Start humidity
    // 100% humidity means the humidity sensor has gotten wet and is no longer useful
    if ( humidity2 > 0 && humidity2 < 100) {

      if (sht31ErrorH) {  // If the SHT-31 has a problem report this humidity otherwise let the SHT-31 report it's humidity
        humidity = humidity2;
      }

      if (bme280ErrorH) {
        if (bme280ErrorH >= error_count_log) {
          logString = "BME280 humidity recovered: " + String(humidity2) + "%";
          mqttLog(app_name_sensors, logString);
        }
        bme280ErrorH = 0;
      }
    } else {
      bme280ErrorH++;
      if (bme280ErrorH == error_count_log) {
        logString = "BME280 humidity error: " + String(humidity2) + "%";
        mqttLog(app_name_sensors, logString);
      }
    }
    // End humidity

    // Start pressure
    // The lowest recorded pressure is 870hpa and the highest is 1085 so if it's outside this then it's a false reading
    if ( (sealevel2 > 850) && (sealevel2 < 1100) ) {

      pressure     = pressure2;
      sealevel     = sealevel2;

      if (bme280ErrorP) {
        if (bme280ErrorP >= error_count_log) {
          logString = "BME280 pressure recovered: " + String(sealevel2) + "hpa";
          mqttLog(app_name_sensors, logString);
        }
        bme280ErrorP = 0;
      }
    } else {  // Bad pressure reading
      bme280ErrorP++;
      if (bme280ErrorP == error_count_log) {
        logString = "BME280 pressure error: " + String(sealevel2) + "hpa";
        mqttLog(app_name_sensors, logString);
      }
      pressure = 0;   // This is a bad pressure so set it to 0 so it's ignored later
    }
    // End pressure

  }
  // End BME280


  if (temperature != 1000) {
    mqttSend(String("temperature/celsius"),   String(temperature), false);
  } else {
    mqttSend(String("temperature/celsius"),   str_NaN,          false);
  }

  if (humidity != 1000) {
    mqttSend(String("humidity/percentage"),   String(humidity), false);
  } else {
    mqttSend(String("humidity/percentage"),   str_NaN,          false);
  }

  if ( (temperature != 1000) && (humidity != 1000) ) {
    dewpoint = 5351/(5351/(temperature + 273.15) - log(humidity/100.0)) - 273.15;
    mqttSend(String("dewpoint/celsius"),      String(dewpoint), false);
  } else {
    mqttSend(String("dewpoint/celsius"),      str_NaN,          false);
  }

  if (pressure != 0) {
    mqttSend(String("pressure/hpa"),          String(pressure), false);
    mqttSend(String("pressure-sealevel/hpa"), String(sealevel), false);
  } else {
    mqttSend(String("pressure/hpa"),          str_NaN,          false);
    mqttSend(String("pressure-sealevel/hpa"), str_NaN,          false);
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

  httpData += trStart + "Pressure:" + tdBreak;
  if (pressure != 0) {
    httpData += String(pressure) + " hPa";
  } else {
    httpData += "-";
  }
  httpData += trEnd;

  httpData += trStart + "Pressure (sea level):" + tdBreak;
  if (sealevel != 0) {
    httpData += String(sealevel) + " hPa";
  } else {
    httpData += "-";
  }
  httpData += trEnd;

  httpData += tableEnd;
  return httpData;
}

String httpSensorSetup() {
  String httpData;
  httpData += trStart + "Elevation (m):" + tdBreak + htmlInput(str_text, "elevation", String(elevation)) + trEnd;
  return httpData;
}

String httpSensorConfig() {
  if (httpServer.hasArg("elevation")) {
    tmpString = String(elevation);
    if (httpServer.arg("elevation") != tmpString) {
      elevation = httpServer.arg("elevation").toFloat();
    }
  }
}

void sensorMqttCallback(char* topic, byte* payload, unsigned int length) {}
void sensorMqttSubs() {}

#endif
