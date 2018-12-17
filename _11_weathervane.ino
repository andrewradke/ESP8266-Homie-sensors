#if FWTYPE == 11      // esp8266-weathervane

const char DIR_ARROW[] PROGMEM = " &nbsp; <span style='transform:rotate({v}deg);position:absolute;color:blue;font-weight:bold;'>&darr;</span>";

const char str_kphPerPulse[]    = "kphPerPulse";
const char str_mmPerPulse[]     = "mmPerPulse";
const char str_secPerGust[]     = "secPerGust";

const char str_kph[]            = " kph";
const char str_mm[]             = " mm";
const char str_html_deg[]       = "&deg;";

const char str_rain_Mm[]        = "rain/mm";
const char str_rain_Counter[]   = "rain_counter/counter";
const char str_wind_Direction[] = "wind_direction/degrees";
const char str_gust_Direction[] = "gust_direction/degrees";
const char str_wind_Speed[]     = "wind_speed/kph";
const char str_gust_Speed[]     = "gust_speed/kph";


void sensorSetup() {
  pinMode(RAINGAUGE_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RAINGAUGE_PIN),  rain, RISING);  // Setup Interrupt
  pinMode(ANEMOMETER_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ANEMOMETER_PIN), wind, RISING);  // Setup Interrupt
  sei();                                                                 // Enable interrupts
}

void  printSensorConfig(const String &cfgStr) {
  mqttSend(String(cfgStr + str_kphPerPulse), String(kphPerPulse), true);
  mqttSend(String(cfgStr + str_mmPerPulse),  String(mmPerPulse),  true);
  mqttSend(String(cfgStr + str_secPerGust),  String(secPerGust),  true);
}

void sensorImportJSON(JsonObject& json) {
  if (json[str_kphPerPulse].is<float>()) {
    kphPerPulse     = json[str_kphPerPulse];
  }
  if (json[str_mmPerPulse].is<float>()) {
    mmPerPulse      = json[str_mmPerPulse];
  }
  if (json[str_secPerGust].is<int>()) {
    secPerGust      = json[str_secPerGust];
  }
}

void sensorExportJSON(JsonObject& json) {
  json[str_kphPerPulse] = kphPerPulse;
  json[str_mmPerPulse]  = mmPerPulse;
  json[str_secPerGust]  = secPerGust;
}

bool sensorUpdateConfig(const String &key, const String &value) {
  if ( key == str_kphPerPulse ) {
    kphPerPulse = value.toFloat();
  } else if ( key == str_mmPerPulse ) {
    mmPerPulse  = value.toFloat();
  } else if ( key == str_secPerGust ) {
    secPerGust  = value.toInt();
  } else {
    return false;
  }
  return true;
}

bool sensorRunAction(const String &key, const String &value) {
  return false;
}

void calcData() {
  // Every second, calculate and print litres/hour
  if ( currentTime >= (cloopTime + 1000) ) {
    cloopTime = currentTime;                      // Updates cloopTime

    voltage = analogRead(WINDVANE_PIN);
    if (voltage < 70) {               // ERROR - Shorted to ground
      dir_reading = 16;
      logString = F("ERROR: Wind vane shorted to ground.");
      mqttLog(app_name_sensors, logString);
    } else if (voltage < 84) {
      dir_reading = 5;
    } else if (voltage < 98) {
      dir_reading = 3;
    } else if (voltage < 119) {
      dir_reading = 4;
    } else if (voltage < 168) {
      dir_reading = 7;
    } else if (voltage < 229) {
      dir_reading = 6;
    } else if (voltage < 282) {
      dir_reading = 9;
    } else if (voltage < 366) {
      dir_reading = 8;
    } else if (voltage < 455) {
      dir_reading = 1;
    } else if (voltage < 552) {
      dir_reading = 2;
    } else if (voltage < 637) {
      dir_reading = 11;
    } else if (voltage < 690) {
      dir_reading = 10;
    } else if (voltage < 768) {
      dir_reading = 15;
    } else if (voltage < 831) {
      dir_reading = 0;
    } else if (voltage < 883) {
      dir_reading = 13;
    } else if (voltage < 941) {
      dir_reading = 14;
    } else if (voltage < 1000) {
      dir_reading = 12;
    } else {                          // ERROR - shorted to 5V
      dir_reading = 17;
      logString = F("ERROR: Wind vane shorted to 5V.");
      mqttLog(app_name_sensors, logString);
    }
    wind_dirs[wind_array_pos]   = dir_reading;

    if (dir_reading > 15) {           // Error reported so put the previous reading into the array so that later averages won't break when the problem is fixed
      if (wind_array_pos == 0) {
        if (wind_array_full) {
          wind_dirs[wind_array_pos] = wind_dirs[WIND_SECS-1];
        } else {
          wind_dirs[wind_array_pos] = 0; // We need to put something in here and we have nothing better to use. We have a problem with the sensor anyway so don't worry any further in code.
        }
      } else {
        wind_dirs[wind_array_pos] = wind_dirs[wind_array_pos-1];
      }
    }


    noInterrupts();                   // We don't want wind_counter changing here while we do the calculation
    wind_speeds[wind_array_pos] = wind_counter;
    wind_counter = 0;                 // Reset counter
    interrupts();


    uint32_t sum_speed = 0;
    uint8_t  min_speed = wind_speeds[wind_array_pos];
    uint8_t  max_speed = 0;
    uint32_t sum_gust  = 0;
    uint8_t  max_dir   = 0;
    uint32_t sum_dirs  = 0;
    uint16_t readings  = WIND_SECS;
    if (! wind_array_full) {
      readings = wind_array_pos+1;    // Only use the number of readings we currently have made to calculate the average (first reading is 0, hence +1)
    }

    for (uint16_t i=0; i<readings; i++) {
      sum_speed += wind_speeds[i];
      sum_dirs  += wind_dirs[i];

#ifdef USEALLGUSTS
      // Find the maximum and minimum wind speeds from the previous 60 seconds
      if (
        ( (wind_array_pos >= 59) && (i > (wind_array_pos - 59)) && (i <= wind_array_pos) ) ||
        ( (wind_array_pos < 59) && (i <= wind_array_pos) ) ||
        ( (wind_array_pos < 59) && (i >= (readings - 59 + wind_array_pos)) )
        ) {
        if (wind_speeds[i] > max_speed) {
          max_speed = wind_speeds[i];
          max_dir   = wind_dirs[i];
          if ( secPerGust > 1 ) {
            sum_gust  = 0;
            for (uint16_t j=0; j<secPerGust; j++) {
              uint16_t k = i+j;
              if ( k >= readings ) {
                k -= readings;     // Wrap back around to the top end of the array
              }
              sum_gust += wind_speeds[k];
            }
          }
        }
      }
#else
      // Find the maximum and minimum wind speeds from the entire array
      if (wind_speeds[i] > max_speed) {
        max_speed = wind_speeds[i];
        max_dir   = wind_dirs[i];
        if ( secPerGust > 1 ) {
          sum_gust  = 0;
          for (uint16_t j=0; j<secPerGust; j++) {
            uint16_t k = i+j;
            if ( k >= readings ) {
              k -= readings;     // Wrap back around to the top end of the array
            }
            sum_gust += wind_speeds[k];
          }
        }
      }
      if (wind_speeds[i] < min_speed) {
        min_speed = wind_speeds[i];
      }
#endif

    }

    if ( secPerGust > 1 ) {
      max_speed = sum_gust / secPerGust;
    }

    wind_speed = (sum_speed / (float) readings) * kphPerPulse;  // Calculated to kph
    wind_dir   = (sum_dirs  / (float) readings) * 22.5;         // Each position on the wind vane is 22.5 degrees


#ifdef USEALLGUSTS
    // Report the gusts as the max speeds
    gust_speed = max_speed * kphPerPulse;
    gust_dir   = max_dir * 22.5;

#else
    // Only report a gust if the wind speed has exceeded 16 knots and there is more than 9 knots from min to max speeds
    // http://wiki.wunderground.com/index.php/Educational_-_Wind_gusts_vs._gust_speed
    gust_speed = wind_speed;  // Default to the current wind and change it below if a gust is detected
    gust_dir   = wind_dir;
    if (wind_speed > 29.632) {                // over 16 knots
      if ((max_speed - min_speed) > 16.668) { // more than 9 knots difference
        // We've got a gust to report
        gust_speed = max_speed * kphPerPulse;
        gust_dir   = max_dir * 22.5;
        logString = String(F("Wind gust: ") + String(gust_speed) + + str_kph + ", " + String(gust_dir) + F(" degrees");
        mqttLog(app_name_sensors, logString);
      }
    }
#endif

    wind_array_pos++;
    if (wind_array_pos >= WIND_SECS) {
      wind_array_pos  = 0;
      wind_array_full = true;
    }

  }
}

void sendData() {
  rain_mm = rain_counter * mmPerPulse;
  mqttSend(String(str_rain_Mm),          String(rain_mm),      false);
  mqttSend(String(str_rain_Counter),     String(rain_counter), false);
  if (dir_reading <= 15) {                // No errors reported
    mqttSend(String(str_wind_Direction), String(wind_dir),     false);
    mqttSend(String(str_gust_Direction), String(gust_dir),     false);
  } else {
    mqttSend(String(str_wind_Direction), str_NaN,              false);
    mqttSend(String(str_gust_Direction), str_NaN,              false);
  }
  mqttSend(String(str_wind_Speed),       String(wind_speed),   false);
  mqttSend(String(str_gust_Speed),       String(gust_speed),   false);
}

void rain () {                     // Interrupt function
  static unsigned long last_interrupt_time = 0;
  unsigned long        interrupt_time      = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 200) {
    rain_counter++;
    last_interrupt_time = interrupt_time;
  }
}
void wind () {                     // Interrupt function
  wind_counter++;
}

String httpSensorData() {
  String httpData = tableStart;

  httpData += trStart + F("Rain:") + tdBreak;
  httpData += String(rain_mm) + str_mm;
  httpData += trEnd;

  httpData += trStart + F("Rain counter:") + tdBreak;
  httpData += String(rain_counter);
  httpData += trEnd;

  httpData += trStart + F("Wind direction:") + tdBreak;
  if (dir_reading <= 15) {
    httpData += String(wind_dir) + str_html_deg;
    tmpString = FPSTR(DIR_ARROW);
    tmpString.replace("{v}", String(wind_dir));
    httpData += tmpString;
  } else {
    httpData += "-";
  }
  httpData += trEnd;

  httpData += trStart + F("Wind speed:") + tdBreak;
  httpData += String(wind_speed) + str_kph;
  httpData += trEnd;

  httpData += trStart + F("Gust direction:") + tdBreak;
  if (dir_reading <= 15) {
    httpData += String(gust_dir) + str_html_deg;
    tmpString = FPSTR(DIR_ARROW);
    tmpString.replace("{v}", String(gust_dir));
    httpData += tmpString;
  } else {
    httpData += "-";
  }
  httpData += trEnd;

  httpData += trStart + F("Gust speed:") + tdBreak;
  httpData += String(gust_speed) + str_kph;
  httpData += trEnd;

  return httpData;
}

String httpSensorSetup() {
  String httpData;
  httpData += trStart + F("Wind KPH per pulse:") + tdBreak + htmlInput(str_text, str_kphPerPulse, String(kphPerPulse)) + trEnd;
  httpData += trStart + F("mm rain per pulse:")  + tdBreak + htmlInput(str_text, str_mmPerPulse,  String(mmPerPulse))  + trEnd;
  httpData += trStart + F("Average gust over this many seconds:")  + tdBreak + htmlInput(str_text, str_secPerGust,  String(secPerGust))  + trEnd;
  return httpData;
}

String httpSensorConfig() {
  if (httpServer.hasArg(str_kphPerPulse)) {
    tmpString = String(kphPerPulse);
    if (httpServer.arg(str_kphPerPulse) != tmpString) {
      kphPerPulse = httpServer.arg(str_kphPerPulse).toFloat();
    }
  }
  if (httpServer.hasArg(str_mmPerPulse)) {
    tmpString = String(mmPerPulse);
    if (httpServer.arg(str_mmPerPulse) != tmpString) {
      mmPerPulse = httpServer.arg(str_mmPerPulse).toFloat();
    }
  }
  if (httpServer.hasArg(str_secPerGust)) {
    tmpString = String(secPerGust);
    if (httpServer.arg(str_secPerGust) != tmpString) {
      secPerGust = httpServer.arg(str_secPerGust).toInt();
    }
  }
}

void sensorMqttCallback(char* topic, byte* payload, unsigned int length) {}
void sensorMqttSubs() {}

#endif
