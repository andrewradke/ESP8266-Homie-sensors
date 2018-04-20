#if FWTYPE == 12      // esp8266-mq135

void sensorSetup() {
}

void  printSensorConfig(const String &cfgStr) {
  mqttSend(String(cfgStr + "mq135_ro"), String(mq135_ro), true);
}

void sensorImportJSON(JsonObject& json) {
  if (json["mq135_ro"].is<int>()) {
    mq135_ro = json["mq135_ro"];
  }
}

void sensorExportJSON(JsonObject& json) {
  json["mq135_ro"] = mq135_ro;
}

bool sensorUpdateConfig(const String &key, const String &value) {
// return true if any config item matches, false if none match
  if ( key == "mq135_ro" ) {
    mq135_ro = value.toInt();
  } else {
    return false;
  }
  return true;
}

bool sensorRunAction(const String &key, const String &value) {
// return true if any action matches, false if none match
  return false;
}


void calcData() {
}

void sendData() {
  double valr = analogRead(SENSOR_PIN);     // Get AIQ value
  val =  ((float)22000*(1023-valr)/valr); 
  // during clean air calibration, read the Ro value and replace MQ135_DEFAULTRO value with it, you can even deactivate following function call.
//  mq135_ro = mq135_getro(val, MQ135_DEFAULTPPM);
  // convert to ppm (using default ro)
  valAIQ = mq135_getppm(val, mq135_ro);

//  Serial.print ( "Val / Ro / value: ");
//  Serial.print ( val);
//  Serial.print ( " / ");
//  Serial.print ( mq135_getro(val, MQ135_DEFAULTPPM) );
//  Serial.print ( " / ");
//  Serial.println ( valAIQ);

//  mqttSend(String("co2/ppm"),   String(MQ135_DEFAULTPPM+(int)ceil(valAIQ)), false);
  mqttSend(String("co2/ppm"),   String((int)ceil(valAIQ)), false);
}

String httpSensorData() {
  String httpData = tableStart;
  httpData += trStart + F("CO&sup2;:") + tdBreak;
//  httpData += String(MQ135_DEFAULTPPM+(int)ceil(valAIQ)) + " ppm";
  httpData += String((int)ceil(valAIQ)) + " ppm";
  httpData += trEnd;
  httpData += tableEnd;
  return httpData;
}

String httpSensorSetup() {
  String httpData;
  httpData += trStart + F("MQ135 RO:") + tdBreak + htmlInput("text", "mq135_ro", String(mq135_ro)) + F("<br/>current reading: ") + String(mq135_getro(val, MQ135_DEFAULTPPM)) + trEnd;
  return httpData;
}

String httpSensorConfig() {
  if (httpServer.hasArg("mq135_ro")) {
    tmpString = String(mq135_ro);
    if (httpServer.arg("mq135_ro") != tmpString) {
      mq135_ro = httpServer.arg("mq135_ro").toInt();
    }
  }
}


/*
 * get the calibrated ro based upon read resistance, and a know ppm
 */
long mq135_getro(long resvalue, double ppm) {
  return (long)(resvalue * exp( log(MQ135_SCALINGFACTOR/ppm) / MQ135_EXPONENT ));
}

/*
 * get the ppm concentration
 */
double mq135_getppm(long resvalue, long ro) {
  double ret = 0;
  double validinterval = 0;
  validinterval = resvalue/(double)ro;
  if (validinterval<MQ135_MAXRSRO && validinterval>MQ135_MINRSRO) {
    ret = (double)((double)MQ135_SCALINGFACTOR * pow( ((double)resvalue/ro), MQ135_EXPONENT));
  }
  return ret;
}

/*****************************  MQGetPercentage **********************************
Input:   rs_ro_ratio - Rs divided by Ro
         pcurve      - pointer to the curve of the target gas
Output:  ppm of the target gas
Remarks: By using the slope and a point of the line. The x(logarithmic value of ppm) 
         of the line could be derived if y(rs_ro_ratio) is provided. As it is a 
         logarithmic coordinate, power of 10 is used to convert the result to non-logarithmic 
         value.
************************************************************************************/ 
int  MQGetPercentage(float rs_ro_ratio, float ro, float *pcurve) {
  return (double)(pcurve[0] * pow(((double)rs_ro_ratio/ro), pcurve[1]));
}
#endif
