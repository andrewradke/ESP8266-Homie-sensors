#if FWTYPE == 8      // esp8266-bme280

void sensorSetup() {
  app_name = "Sensors";
#ifdef USESYSLOG
  syslog.appName(app_name);
#endif
  sensorStatus = bme.begin(0x76);
  logString = "BME280 ";
  if (sensorStatus) {
    // The following are the recommended values from shapter 3.5 of the BME280 datasheet with
    // the expeception that the sampling rate is governed by the mqtt_interval value 
    // forced mode, 1x temperature / 1x humidity / 1x pressure oversampling, filter off
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1, // temperature
                    Adafruit_BME280::SAMPLING_X1, // pressure
                    Adafruit_BME280::SAMPLING_X1, // humidity
                    Adafruit_BME280::FILTER_OFF   );
  } else {
    logString = logString + " NOT";
  }
  logString = logString + "found";
  printMessage(logString, true);
#ifdef USESYSLOG
  syslog.log(LOG_INFO, logString);
#endif
}

void  printSensorConfig(String cfgStr) {
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

bool sensorUpdateConfig(String key, String value) {
  if ( key == "elevation" ) {
    elevation = value.toFloat();
  } else {
    return false;
  }
  return true;
}

bool sensorRunAction(String key, String value) {
  return false;
}

void calcData() {
}

void sendData() {
  app_name = "Sensors";
  // Start BME280
  if (! bme280Status) {
    logString = "Retrying BME280";
    mqttLog(logString);

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
      mqttLog(logString);
    }

  } else {
    // Only needed in forced mode! In normal mode, you can remove the next line. Normal mode can result in higher temperatures due to the BME280 getting heating up from continuous readings.
    bme.takeForcedMeasurement(); // has no effect in normal mode

    temperature  = bme.readTemperature();
    humidity     = bme.readHumidity();
    pressure     = bme.readPressure() / 100.0;
//  The following equation is from https://www.sandhurstweather.org.uk/barometric.pdf
    sealevel     = pressure / exp(-elevation/((temperature2 + 273.15)*29.263));


    // Start temperature
    // The lowest recorded temperature is -88C and the highest is 58C so if it's outside this then it's a false reading
    if ( temperature > -100 && temperature < 65) {

      mqttSend(String("temperature/celsius"),   String(temperature), false);

      if (bme280ErrorT) {
        if (bme280ErrorT >= error_count_log) {
          logString = "BME280 temperature recovered: " + String(temperature) + "C";
          mqttLog(logString);
        }
        bme280ErrorT = 0;
      }
    } else {  // Bad temperature reading
      bme280ErrorT++;
      if (bme280ErrorT == error_count_log) {
        logString = "BME280 temperature error: " + String(temperature) + "C";
        mqttLog(logString);
      }
      mqttSend(String("temperature/celsius"),   String("nan"), false);
    }
    // End temperature


    // Start humidity
    // 100% humidity means the humidity sensor has gotten wet and is no longer useful
    if ( humidity2 > 0 && humidity2 < 100) {

      mqttSend(String("humidity/percentage"),   String(humidity), false);

      if (bme280ErrorH) {
        if (bme280ErrorH >= error_count_log) {
          logString = "BME280 humidity recovered: " + String(humidity) + "%";
          mqttLog(logString);
        }
        bme280ErrorH = 0;
      }
    } else {
      bme280ErrorH++;
      if (bme280ErrorH == error_count_log) {
        logString = "BME280 humidity error: " + String(humidity) + "%";
        mqttLog(logString);
      }
      mqttSend(String("humidity/percentage"),   String("nan"), false);
    }
    // End humidity


    // Start pressure
    // The lowest recorded pressure is 870hpa and the highest is 1085 so if it's outside this then it's a false reading
    if ( (sealevel > 850) && (sealevel < 1100) ) {

      mqttSend(String("pressure/hpa"),          String(pressure), false);
      mqttSend(String("pressure-sealevel/hpa"), String(sealevel), false);

      if (bme280ErrorP) {
        if (bme280ErrorP >= error_count_log) {
          logString = "BME280 pressure recovered: " + String(sealevel) + "hpa";
          mqttLog(logString);
        }
        bme280ErrorP = 0;
      }
    } else {  // Bad pressure reading
      bme280ErrorP++;
      if (bme280ErrorP == error_count_log) {
        logString = "BME280 pressure error: " + String(sealevel) + "hpa";
        mqttLog(logString);
      }
      mqttSend(String("pressure/hpa"),          String("nan"), false);
      mqttSend(String("pressure-sealevel/hpa"), String("nan"), false);
    }
    // End pressure

  }
  // End BME280
}

#endif
