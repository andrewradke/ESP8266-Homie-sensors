#if FWTYPE == 1      // esp8266-temperature-sensor

void sensorSetup() {
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
  temperature = getTemperature();
}

void sendData() {
  if ( temperature != 1000) {
    mqttSend(String("temperature/celsius"), String(temperature), false);
  } else {
#ifdef DEBUG
    dmesg();
    Serial.println("No valid temperature returned.");
#endif
    mqttTime = currentTime - (1000 * (mqtt_interval - 1 ) );                       // only wait 1 second before trying again when measurement failed
  }
}

String httpSensorData() {
  String httpData = "<table>";
  String trStart = "<tr><td>";
  String tdBreak = "</td><td>";
  String trEnd   = "</td></tr>";

  httpData += trStart + "Temperature:" + tdBreak;
  if (temperature != 1000) {
    httpData += String(temperature) + " C";
  } else {
    httpData += "-";
  }
  httpData += trEnd;

  httpData += "</table>";
  return httpData;
}

float getTemperature() {
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius;

  ds.reset_search();
  delay(250);

  while ( ds.search(addr)) {
    if (OneWire::crc8(addr, 7) != addr[7]) {
#ifdef DEBUG
      dmesg();
      Serial.println("CRC error for 1wire address.");
#endif
      continue;
    }

    // the first ROM byte indicates which chip
    switch (addr[0]) {
      case 0x10:
        // Serial.println("  Chip = DS18S20");  // or old DS1820
        type_s = 1;
        break;
      case 0x28:
        // Serial.println("  Chip = DS18B20");
        type_s = 0;
        break;
      case 0x22:
        // Serial.println("  Chip = DS1822");
        type_s = 0;
        break;
      default:
#ifdef DEBUG
        Serial.println("Device is not a DS18x20 family device.");
#endif
        continue;
    }

    ds.reset();
    ds.select(addr);
    ds.write(0x44, 0);                          // start conversion, with parasite power off

    present = ds.reset();
    ds.select(addr);
    ds.write(0xBE);                             // Read Scratchpad

    for ( i = 0; i < 9; i++) {                  // we need 9 bytes
      data[i] = ds.read();
    }

    // Convert the data to actual temperature because the result is a 16 bit signed integer, it should
    // be stored to an "int16_t" type, which is always 16 bits even when compiled on a 32 bit processor.
    int16_t raw = (data[1] << 8) | data[0];
    if (type_s) {
      raw = raw << 3;                           // 9 bit resolution default
      if (data[7] == 0x10) {
        raw = (raw & 0xFFF0) + 12 - data[6];    // "count remain" gives full 12 bit resolution
      }
    } else {
      byte cfg = (data[4] & 0x60);
      // at lower res, the low bits are undefined, so let's zero them
      if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
      else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
      else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
      //// default is 12 bit resolution, 750 ms conversion time
    }
    if (raw == 1360) {
#ifdef DEBUG
      dmesg();
      Serial.println("85C is the startup temperature and invalid.");
#endif
      continue;
    }
    celsius = (float)raw / 16.0;

    return celsius;                 // Return as soon as we have the first temperature sensor reading, multiple sensors not yet supported
  }
  return 1000;                      // We didn't get any temperatures so return 1000 as an error value.
}

#endif
