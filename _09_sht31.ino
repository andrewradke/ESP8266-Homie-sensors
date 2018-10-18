#if FWTYPE == 9      // esp8266-sht31

const char str_humidity100Okay[] = "humidity100Okay";


void sensorSetup() {
  sensorStatus = sht31.begin(0x44);   // Set to 0x45 for alternate i2c addr
  // The library always returns true for begin() so it's not much point checking the returned value
  logString = F("SHT-31 ");
  uint16_t sht31State = sht31.readStatus();
  if (sht31State == 65535) {          // returns 65535 if no sensor (no I2C device at all?) found
    logString    = logString + F("NOT ");
    sensorError  = true;
    sensorStatus = false;
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
  mqttSend(String(cfgStr + str_humidity100Okay), String(humidity100Okay), true);
}

void sensorImportJSON(JsonObject& json) {
  if (json[str_humidity100Okay].is<bool>()) {
    humidity100Okay = json[str_humidity100Okay];
  }
}

void sensorExportJSON(JsonObject& json) {
  json[str_humidity100Okay] = humidity100Okay;
}
bool sensorUpdateConfig(const String &key, const String &value) {
  if ( key == str_humidity100Okay ) {
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

  // Start SHT-31
  if (! sensorStatus) {
    logString = "Retrying SHT-31";
    mqttLog(app_name_sensors, logString);

    sensorStatus  = sht31.begin();
    uint16_t sht31State = sht31.readStatus();
    if (sht31State != 65535) {          // returns 65535 if no sensor (no I2C device at all?) found
      logString = "SHT-31 found";
      sensorError = false;
      sensorStatus = true;
      mqttLog(app_name_sensors, logString);
    } else {
      sensorStatus = false;
    }

  } else {

    readTemperature = sht31.readTemperature();
    readHumidity    = sht31.readHumidity();


    // Start temperature
    // The lowest recorded temperature is -88C and the highest is 58C so if it's outside this then it's a false reading
    if ( readTemperature > -100 && readTemperature < 65) {

      temperature = readTemperature;

      if (sensorErrorT) {
        if (sensorErrorT >= error_count_log) {
          logString = "SHT-31 temperature recovered: " + String(readTemperature) + "C";
          mqttLog(app_name_sensors, logString);
        }
        sensorErrorT = 0;
      }
    } else {  // Bad temperature reading
      sensorErrorT++;
      if (sensorErrorT == error_count_log) {
        logString = "SHT-31 temperature error: " + String(readTemperature) + " C";
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
          logString = "SHT-31 humidity recovered: " + String(readHumidity) + "%";
          mqttLog(app_name_sensors, logString);
        }
        sensorErrorH = 0;
      }

    } else {  // Bad humidity reading
      sensorErrorH++;
      if (sensorErrorH == error_count_log) {
        logString = "SHT-31 humidity error: " + String(readHumidity) + " %";
        mqttLog(app_name_sensors, logString);
      }
    }
    // End humidity

  }
  // End SHT-31


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
  httpData += trStart + F("100% humidity okay?") + tdBreak + htmlCheckbox(str_humidity100Okay, humidity100Okay)  + trEnd;
  return httpData;
}

String httpSensorConfig() {
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
