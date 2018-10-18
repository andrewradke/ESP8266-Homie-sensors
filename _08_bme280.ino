#if FWTYPE == 8      // esp8266-bme280

const char str_elevation[]       = "elevation";
const char str_humidity100Okay[] = "humidity100Okay";


void sensorSetup() {
  sensorStatus = bme.begin(I2C_ADDRESS);
  logString = F("BME280 ");
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
    logString = logString + F("NOT ");
  }
  logString = logString + F("found");
  mqttLog(app_name_sensors, logString);
}

void  printSensorConfig(const String &cfgStr) {
  mqttSend(String(cfgStr + str_elevation),       String(elevation),       true);
  mqttSend(String(cfgStr + str_humidity100Okay), String(humidity100Okay), true);
}

void sensorImportJSON(JsonObject& json) {
  if (json[str_elevation].is<float>()) {
    elevation = json[str_elevation];
  }
  if (json[str_humidity100Okay].is<bool>()) {
    humidity100Okay = json[str_humidity100Okay];
  }
}

void sensorExportJSON(JsonObject& json) {
  json[str_elevation]       = elevation;
  json[str_humidity100Okay] = humidity100Okay;
}

bool sensorUpdateConfig(const String &key, const String &value) {
  if ( key == str_elevation ) {
    elevation = value.toFloat();
  } else if ( key == str_humidity100Okay ) {
    if ( value.equalsIgnoreCase("true") or value.equalsIgnoreCase("yes") or value == "1" ) {
      humidity100Okay = true;
    } else if ( value.equalsIgnoreCase("false") or value.equalsIgnoreCase("no") or value == "0" ) {
      humidity100Okay = false;
    } else {
      logString = String(F("Unrecognised value for humidity100Okay: ")) + value;
      mqttLog(app_name_sensors, logString);      
    }
  } else {
    return false;
  }
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

  // Start BME280
  if (! sensorStatus) {
    logString = "Retrying BME280";
    mqttLog(app_name_sensors, logString);

    sensorStatus = bme.begin();
    if (sensorStatus) {
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

    readTemperature = bme.readTemperature();
    readHumidity    = bme.readHumidity();
    readPressure    = bme.readPressure() / 100.0;
//  The following equation is from https://www.sandhurstweather.org.uk/barometric.pdf
    readSeaLevel    = readPressure / exp(-elevation/((readTemperature + 273.15)*29.263));


    // Start temperature
    // The lowest recorded temperature is -88C and the highest is 58C so if it's outside this then it's a false reading
    if ( readTemperature > -100 && readTemperature < 65) {

      temperature = readTemperature;

      if (sensorErrorT) {
        if (sensorErrorT >= error_count_log) {
          logString = "BME280 temperature recovered: " + String(readTemperature) + "C";
          mqttLog(app_name_sensors, logString);
        }
        sensorErrorT = 0;
      }
    } else {  // Bad temperature reading
      sensorErrorT++;
      if (sensorErrorT == error_count_log) {
        logString = "BME280 temperature error: " + String(readTemperature) + " C";
        mqttLog(app_name_sensors, logString);
      }
    }
    // End temperature


    // Start humidity
    // 100% humidity can mean the humidity sensor has gotten wet and is no longer useful
    if ( readHumidity > 0 && ( ( humidity100Okay && readHumidity == 100 ) || readHumidity < 100 ) ) {

      humidity = readHumidity;

      if (sensorErrorH) {
        if (sensorErrorH >= error_count_log) {
          logString = "BME280 humidity recovered: " + String(readHumidity) + "%";
          mqttLog(app_name_sensors, logString);
        }
        sensorErrorH = 0;
      }

    } else {  // Bad humidity reading
      sensorErrorH++;
      if (sensorErrorH == error_count_log) {
        logString = "BME280 humidity error: " + String(readHumidity) + " %";
        mqttLog(app_name_sensors, logString);
      }
    }
    // End humidity


    // Start pressure
    // The lowest recorded pressure is 870hpa and the highest is 1085 so if it's outside this then it's a false reading
    if ( (readSeaLevel > 850) && (readSeaLevel < 1100) ) {

      pressure     = readPressure;
      sealevel     = readSeaLevel;

      if (sensorErrorP) {
        if (sensorErrorP >= error_count_log) {
          logString = "BME280 pressure recovered: " + String(readSeaLevel) + "hpa";
          mqttLog(app_name_sensors, logString);
        }
        sensorErrorP = 0;
      }
    } else {  // Bad pressure reading
      sensorErrorP++;
      if (sensorErrorP == error_count_log) {
        logString = "BME280 pressure error: " + String(readSeaLevel) + " hpa";
        mqttLog(app_name_sensors, logString);
      }
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
  httpData += trStart + F("Elevation (m):")      + tdBreak + htmlInput(str_text, str_elevation, String(elevation)) + trEnd;
  httpData += trStart + F("100% humidity okay?") + tdBreak + htmlCheckbox(str_humidity100Okay, humidity100Okay)  + trEnd;
  return httpData;
}

String httpSensorConfig() {
  if (httpServer.hasArg(str_elevation)) {
    tmpString = String(elevation);
    if (httpServer.arg(str_elevation) != tmpString) {
      elevation = httpServer.arg(str_elevation).toFloat();
    }
  }
  if (httpServer.hasArg(str_humidity100Okay)) {
    if (httpServer.arg(str_humidity100Okay) == str_on) {
      humidity100Okay = true;
    } else {
      humidity100Okay = false;
    }
  }
}

void sensorMqttCallback(char* topic, byte* payload, unsigned int length) {}
void sensorMqttSubs() {}

#endif
