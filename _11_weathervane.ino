#if FWTYPE == 11      // esp8266-weathervane

void sensorSetup() {
  pinMode(RAINGAUGE_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RAINGAUGE_PIN),  rain, RISING);  // Setup Interrupt
  pinMode(ANEMOMETER_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ANEMOMETER_PIN), wind, RISING);  // Setup Interrupt
  sei();                                                                 // Enable interrupts
}

void  printSensorConfig(String cfgStr) {
  mqttSend(String(cfgStr + "kphPerPulse"), String(kphPerPulse), true);
  mqttSend(String(cfgStr + "mmPerPulse"),  String(mmPerPulse),  true);
}

void sensorImportJSON(JsonObject& json) {
  if (json["kphPerPulse"].is<float>()) {
    kphPerPulse     = json["kphPerPulse"];
  }
  if (json["mmPerPulse"].is<float>()) {
    mmPerPulse      = json["mmPerPulse"];
  }
}

void sensorExportJSON(JsonObject& json) {
  json["kphPerPulse"] = kphPerPulse;
  json["mmPerPulse"]  = mmPerPulse;
}

bool sensorUpdateConfig(String key, String value) {
  if ( key == "kphPerPulse" ) {
    kphPerPulse = value.toFloat();
  } else if ( key == "mmPerPulse" ) {
    mmPerPulse  = value.toFloat();
  } else {
    return false;
  }
  return true;
}

bool sensorRunAction(String key, String value) {
  return false;
}

void calcData() {
  app_name = "Sensors";
  // Every second, calculate and print litres/hour
  if ( currentTime >= (cloopTime + 1000) ) {
    cloopTime = currentTime;                      // Updates cloopTime

    voltage = analogRead(WINDVANE_PIN);
    if (voltage < 70) {               // ERROR - Shorted to ground
      dir_reading = 16;
      logString = "ERROR: Wind vane shorted to ground.";
      mqttLog(logString);
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
      logString = "ERROR: Wind vane shorted to 5V.";
      mqttLog(logString);
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
        ( (wind_array_pos >= 59) && (i > (wind_array_pos - 59)) ) ||
        ( (wind_array_pos < 59) && (i <= wind_array_pos) ) ||
        ( (wind_array_pos < 59) && (i >= (readings - 59 + wind_array_pos)) )
        ) {
        if (wind_speeds[i] > max_speed) {
          max_speed = wind_speeds[i];
          max_dir   = wind_dirs[i];
        }
      }
#else
      // Find the maximum and minimum wind speeds from the entire array
      if (wind_speeds[i] > max_speed) {
        max_speed = wind_speeds[i];
        max_dir   = wind_dirs[i];
      }
      if (wind_speeds[i] < min_speed) {
        min_speed = wind_speeds[i];
      }
#endif

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
        logString = "Wind gust: " + String(gust_speed) + " kph, " + String(gust_dir) + " degrees";
        mqttLog(logString);
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
  mqttSend(String("rain/mm"),                    String(rain_mm),      false);
  mqttSend(String("rain_counter/counter"),       String(rain_counter), false);
  if (dir_reading <= 15) {                // No errors reported
    mqttSend(String("wind_direction/degrees"),   String(wind_dir),     false);
    mqttSend(String("gust_direction/degrees"),   String(gust_dir),     false);
  } else {
    mqttSend(String("wind_direction/degrees"),   strNaN,            false);
    mqttSend(String("gust_direction/degrees"),   strNaN,            false);
  }
  mqttSend(String("wind_speed/kph"),             String(wind_speed),   false);
  mqttSend(String("gust_speed/kph"),             String(gust_speed),   false);
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

  httpData += trStart + "Rain:" + tdBreak;
  httpData += String(rain_mm) + " mm";
  httpData += trEnd;

  httpData += trStart + "Rain counter:" + tdBreak;
  httpData += String(rain_counter);
  httpData += trEnd;

  httpData += trStart + "Wind direction:" + tdBreak;
  if (dir_reading <= 15) {
    httpData += String(wind_dir) + " deg";
  } else {
    httpData += "-";
  }
  httpData += trEnd;

  httpData += trStart + "Wind speed:" + tdBreak;
  httpData += String(wind_speed) + " kph";
  httpData += trEnd;

  httpData += trStart + "Gust direction:" + tdBreak;
  if (dir_reading <= 15) {
    httpData += String(gust_dir) + " deg";
  } else {
    httpData += "-";
  }
  httpData += trEnd;

  httpData += trStart + "Gust speed:" + tdBreak;
  httpData += String(gust_speed) + " kph";
  httpData += trEnd;

  httpData += tableEnd;
  return httpData;
}

String httpSensorSetup() {
  String httpData;
  httpData += trStart + "Wind KPH per pulse:" + tdBreak + htmlInput("text",     "kphPerPulse", String(kphPerPulse)) + trEnd;
  httpData += trStart + "mm rain per pulse:"  + tdBreak + htmlInput("text",     "mmPerPulse",  String(mmPerPulse))  + trEnd;
  return httpData;
}

String httpSensorConfig() {
  if (httpServer.hasArg("kphPerPulse")) {
    tmpString = String(kphPerPulse);
    if (httpServer.arg("kphPerPulse") != tmpString) {
      kphPerPulse = httpServer.arg("kphPerPulse").toFloat();
    }
  }
  if (httpServer.hasArg("mmPerPulse")) {
    tmpString = String(mmPerPulse);
    if (httpServer.arg("mmPerPulse") != tmpString) {
      mmPerPulse = httpServer.arg("mmPerPulse").toFloat();
    }
  }
}

#endif
