#include <FS.h>                   // this needs to be first, or it all crashes and burns...

/*
   FWTYPE:
   1  esp8266-temperature-sensor
   2  esp8266-flow-counter
   3  esp8266-switch
   4  esp8266-depth-sensor
   5  esp8266-pressure-sensor
   6  esp8266-loadcell
   7  esp8266-watermeter
   8  esp8266-bme280
   9  esp8266-sht31
   10 esp8266-air
   11 esp8266-weathervane
*/

/********************  IMPORTANT NOTE for ADS1115  ********************
   Note: change ADS1115_CONVERSIONDELAY from 8 to 9 in libraries/Adafruit_ADS1X15/Adafruit_ADS1015.h
   If this isn't done the value is read before it's converted and you will get the previous pin's reading
  #define ADS1115_CONVERSIONDELAY         (9)
 **********************************************************************/

// Much of the HTTP authentication code is based on brzi's work published at https://www.hackster.io/brzi/esp8266-advanced-login-security-748560


#define FWTYPE     7
#define FWVERSION  "0.9.8"
#define FWPASSWORD "yuruga08"
//#define USESSD1306                // SSD1306 OLED display


//#define DEBUG
#define SERIALSPEED 74880

#include <ESP8266WiFi.h>          // ESP8266 Core WiFi Library

#include <DNSServer.h>            // Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     // Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>

#include <ArduinoJson.h>          // https://github.com/bblanchon/ArduinoJson
#include <PubSubClient.h>         // http://pubsubclient.knolleary.net/


// Accurate time is needed to be able to verify security certificates
#include <time.h>
char     ntp_server1[41] = "0.pool.ntp.org";
char     ntp_server2[41] = "1.pool.ntp.org";

#include <WiFiClientSecure.h>
// Root certificate used by mqtt.home.deepport.net.
// Defined in "CACert" tab.
extern const unsigned char caCert[] PROGMEM;
extern const unsigned int caCertLen;
bool tlsOkay = false;


WiFiClientSecure espSecureClient;
WiFiClient       espClient;


#include <Syslog.h>               // https://github.com/arcao/Syslog
#include <WiFiUdp.h>
bool     use_syslog        = false;
char     host_name[21]     = "";
char     syslog_server[41] = "syslog.local";
uint16_t loglevel          = LOG_NOTICE;
WiFiUDP  udpClient;
Syslog   syslog(udpClient, SYSLOG_PROTO_IETF);
char     *app_name          = ""; // printMessage will use this too so it's always declared


#ifdef USESSD1306
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define OLED_RESET -1
Adafruit_SSD1306 display(OLED_RESET);
#define COLS 20
#define ROWS 8
uint32_t prevDisplay = 0;
char     displayArray[ROWS][COLS+1]; // y postion first in array as we are dealing with lines of text then position on line
uint8_t  displayLine = 0;
uint16_t displaySleep = 30;       // Seconds before the display goes to sleep
#endif


// D7 on a WeMos D1 mini
#define CONFIG_PIN 13


// ########### Sensor Variables ############
#include "SensorVariables.h"



//define the default MQTT values here, if there are different values in config.json, they are overwritten.
char mqtt_server[41]   = "mqtt.local";
bool mqtt_tls          = false;
bool mqtt_auth         = false;
char mqtt_port[6]      = "1883";
char mqtt_name[21];
char mqtt_user[21];
char mqtt_passwd[33];
int  mqtt_interval     = 5;         // interval in seconds between updates
int  mqtt_watchdog     = 60;        // seconds with mqtt data before watchdog reboots device
unsigned long watchdog = millis();  // timer to keep last watchdog packet
const String strNaN    = "nan";

int  error_count_log   = 2;         // How many errors need to be encountered for a sensor before it's logged, if it's more or less don't log


/// Max size for the MQTT data output string
#define OUT_STR_MAX 120
char output[OUT_STR_MAX];

String baseTopic;
char pubTopic[50];

/// The MQTT client
//PubSubClient mqttClient(espSecureClient);
PubSubClient mqttClient;


/// The HTTP server
ESP8266WebServer httpServer(80);
String httpFooter = "</body></html>";
String httpData;
String tableStart = "<table>";
String tableEnd   = "</table>";
String trStart    = "<tr><td>";
String trEnd      = "</td></tr>";
String tdBreak    = "</td><td>";
String thStart    = "<tr><th>";
String thEnd      = "</th></tr>";
String thBreak    = "</th><th>";
bool   lock = false;                // This bool is used to control device lockout 
String httpUser   = "admin";
String httpPasswd = "esp8266";
unsigned long logincld = millis(), reqmillis = millis(), tempign = millis(); // First 2 timers are for lockout and last one is inactivity timer
uint8_t trycount = 0;                                                        // trycount will be our buffer for remembering how many failed login attempts there have been


bool rebootRequired = false;


bool      use_staticip   = false;
char      dns_server[16] = "192.168.0.1";
IPAddress ip(192, 168, 0, 99);
IPAddress dns_ip(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress gateway(192, 168, 0, 1);


String ssid = "";
String psk = "";


unsigned long currentTime     = 0;
unsigned long cloopTime       = 0;
unsigned long mqttTime        = 0;
unsigned long mqttConnectTime = 0;



// flag for whether saving config is required after WiFiManager has updated values
bool shouldSaveConfig = false;


String tmpString;
String logString;     // This can be used by MQTT logging as well



////////////////////////////////////// setup ///////////////////////////////////////

void setup() {
#ifdef DEBUG
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
  // but actually the LED is on; this is because
  // it is acive low on the ESP-01)

  tmpString = String(fwname);
  tmpString = String(tmpString + "-DEBUG");
  tmpString.toCharArray(fwname, 40);
#endif

  Serial.begin(SERIALSPEED);        // Open serial monitor at SERIALSPEED baud to see results.
  delay(200);                       // Give the serial port time to setup


  pinMode(CONFIG_PIN, INPUT_PULLUP);


#ifdef USESSD1306
  Serial.print("Configuring OLED...");
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x64)
  display.setTextSize(1);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  for (byte y=0; y<ROWS; y++) {
    for (byte x=0; x<COLS; x++) {
      displayArray[y][x] = ' ';
    }
  }
  printDone();
#endif

  Serial.println();

  app_name = "SYS";
  logString = String(fwname) + " v" + FWVERSION + " booting.";
  printMessage(logString, true);
  app_name = "NET";
  logString = "Connecting to '" + String(WiFi.SSID()) + "'";
  printMessage(logString, true);



  uint32_t realSize = ESP.getFlashChipRealSize();
  uint32_t ideSize = ESP.getFlashChipSize();
  if (ideSize != realSize) {
    logString = "Flash size config wrong. IDE: " + String(ideSize/1048576.0) + " MB, real: " + String(realSize/1048576.0) + " MB";
    printMessage(logString, true);
    if (ideSize >= realSize) {
      logString = "\nFlash too small!\nCANNOT BOOT\n";
      printMessage(logString, true);
      bool ledState = 0;
      while (1) {
        digitalWrite(LED_BUILTIN, ledState);
        ledState = 1 - ledState;
        delay(1000);
      }      
    }
  }


  loadConfig();


#ifdef DEBUG
  dmesg();
  Serial.println("Saved WiFi credentials");
  dmesg();
  Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
  dmesg();
  Serial.printf("PSK:  %s\n", WiFi.psk().c_str());
#endif


  if ( digitalRead(CONFIG_PIN) == HIGH ) {  // If the config pin IS grounded then skip this
    // Back off for between 0 and 5 seconds before starting Wifi
    // This reduces the sudden current draw when too many sensors start at once or lots of data packets at exactly the same time
    long startupDelay = random(1000);
    logString = "Startup random delay: " + String(startupDelay);
    printMessage(logString, true);

    delay( startupDelay );
  }

  //WiFiManager
  //Local intialization in setup() only. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;


#ifdef DEBUG
  wifiManager.setDebugOutput(true);
#endif


  // is configuration portal requested?
  if ( ( digitalRead(CONFIG_PIN) == LOW ) || (WiFi.SSID() == "") ) {
    if ( digitalRead(CONFIG_PIN) == LOW )
      logString = "Config PIN pressed.";
    if (WiFi.SSID() == "")
      logString = "No saved SSID.";
    logString += " Resetting configuration.";
    printMessage(logString, true);

    // set config save notify callback
    wifiManager.setSaveConfigCallback(saveConfigCallback);

    byte oct1 = dns_ip[0];
    byte oct2 = dns_ip[1];
    byte oct3 = dns_ip[2];
    byte oct4 = dns_ip[3];
    sprintf(dns_server, "%d.%d.%d.%d", oct1, oct2, oct3, oct4);
    WiFiManagerParameter custom_dns_server("dnsserver", "DNS server", dns_server, 16);

    // set static ip
    wifiManager.setSTAStaticIPConfig(ip, gateway, subnet);
    WiFiManagerParameter dns_server_text("DNS server:");
    wifiManager.addParameter(&dns_server_text);
    wifiManager.addParameter(&custom_dns_server);

    WiFiManagerParameter mqtt_server_text("MQTT server:");
    wifiManager.addParameter(&mqtt_server_text);
    WiFiManagerParameter custom_mqtt_server("mqtt_server", "MQTT server", mqtt_server, 40);
    wifiManager.addParameter(&custom_mqtt_server);

    WiFiManagerParameter mqtt_port_text("MQTT server port:");
    wifiManager.addParameter(&mqtt_port_text);
    WiFiManagerParameter custom_mqtt_port("mqtt_port", "MQTT port", mqtt_port, 5);
    wifiManager.addParameter(&custom_mqtt_port);

    WiFiManagerParameter mqtt_name_text("MQTT name:");
    wifiManager.addParameter(&mqtt_name_text);
    WiFiManagerParameter custom_mqtt_name("mqtt_name", "MQTT name", mqtt_name, 20);
    wifiManager.addParameter(&custom_mqtt_name);

    WiFiManagerParameter mqtt_user_text("MQTT user:");
    wifiManager.addParameter(&mqtt_user_text);
    WiFiManagerParameter custom_mqtt_user("mqtt_user", "MQTT user", mqtt_user, 20);
    wifiManager.addParameter(&custom_mqtt_user);

    WiFiManagerParameter mqtt_passwd_text("MQTT password:");
    wifiManager.addParameter(&mqtt_passwd_text);
    WiFiManagerParameter custom_mqtt_passwd("mqtt_passwd", "MQTT password", mqtt_passwd, 32);
    wifiManager.addParameter(&custom_mqtt_passwd);

    WiFiManagerParameter syslog_server_text("syslog server:");
    wifiManager.addParameter(&syslog_server_text);
    WiFiManagerParameter custom_syslog_server("syslog_server", "syslog server", syslog_server, 40);
    wifiManager.addParameter(&custom_syslog_server);


// #################### any extra WM config #defined in variables.h
#ifdef WMADDCONFIG
WMADDCONFIG
#endif


    if (use_staticip) {
      WiFiManagerParameter ip_address_text("IP, gateway, netmask:");
      wifiManager.addParameter(&ip_address_text);
    }


#ifdef USESSD1306
    display.clearDisplay();

    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Setup mode");

    display.setTextSize(1);
    display.print("SSID: ");
    display.println(fwname);
    display.println("IP: 192.168.4.1");

    display.display();
#endif

    // sets 5 minute timeout until configuration portal gets turned off and it reboots
    wifiManager.setConfigPortalTimeout(300);
    wifiManager.setTimeout(300);

    if (!wifiManager.startConfigPortal(fwname, FWPASSWORD)) {
      logString = "Failed to connect and hit timeout. Rebooting.";
      printMessage(logString, true);

      delay(3000);
      //reboot and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(5000);
    }


    //read updated parameters
    if (use_staticip) {
      strcpy(dns_server,  custom_dns_server.getValue());
      dns_ip.fromString(dns_server);
      ip        = WiFi.localIP();
      subnet    = WiFi.subnetMask();
      gateway   = WiFi.gatewayIP();
    }

    strcpy(mqtt_server,   custom_mqtt_server.getValue());
    strcpy(mqtt_port,     custom_mqtt_port.getValue());
    strcpy(mqtt_name,     custom_mqtt_name.getValue());
    strcpy(mqtt_user,     custom_mqtt_user.getValue());
    strcpy(mqtt_passwd,   custom_mqtt_passwd.getValue());

    strcpy(host_name,     custom_mqtt_name.getValue());     // use the MQTT name as the hostname
    strcpy(syslog_server, custom_syslog_server.getValue());


// #################### any extra WM config #defined in variables.h
#ifdef WMGETCONFIG
WMGETCONFIG
#endif


    //save the custom parameters to FS
    if (shouldSaveConfig) {
      saveConfig();
    }
    Serial.println();
  } else {
    //We don't want to stay here. Just timeout and reboot after 10 seconds
    wifiManager.setTimeout(10);

    // fetches ssid and password and tries to connect
    // if it does not connect it starts an access point with the ssid set to fwname
    if (!wifiManager.autoConnect(fwname, FWPASSWORD)) {
      logString = "Failed to connect within timeout. Rebooting.";
      printMessage(logString, true);

      delay(1000);
      //reboot and try again
      ESP.restart();
      delay(5000);
    }
  }

  if (use_staticip) {
    // WiFi.setDNS(dns_server); // This doesn't work with the ESP8266 so reconfigure everything
    WiFi.config(ip, gateway, subnet, dns_ip);
  }


  if (use_syslog) {
    setupSyslog();
  }


  ssid = WiFi.SSID();
  psk = WiFi.psk();

  //if you get here you have connected to the WiFi
  app_name = "WIFI";
  logString = "Connected";
  printMessage(logString, true);

  if (use_syslog) {
    syslog.appName(app_name);
    syslog.log(LOG_INFO, logString);
  }

  app_name = "NET";
  if (use_staticip) {
    logString = "Static";
  } else {
    logString = "DHCP";
  }
  logString += " IP address: " + IPtoString(WiFi.localIP());
  printMessage(logString, true);
  if (use_syslog) {
    syslog.appName(app_name);
    syslog.log(LOG_INFO, logString);
  }

  dmesg();
  Serial.print("Subnet mask: ");
  Serial.println(WiFi.subnetMask());
  dmesg();
  Serial.print("Gateway:     ");
  Serial.println(WiFi.gatewayIP());

  if (use_staticip) {
    dmesg();
    Serial.print("DNS Server:  ");
    Serial.println(dns_server);
  }


  if (use_syslog) {
    syslog.appName("SYS");
    logString = String(fwname) + " v" + FWVERSION;;
    syslog.log(LOG_INFO, logString);
  }


  logString = "Starting web server";
  printMessage(logString, true);

  httpSetup();

   // These 3 lines tell esp to collect User-Agent and Cookie in http header when request is made 
  const char * headerkeys[] = {"User-Agent","Cookie"} ;
  size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
  httpServer.collectHeaders(headerkeys, headerkeyssize );

  httpServer.begin();
  MDNS.begin(host_name);
  MDNS.addService("http", "tcp", 80);
  logString = "Web server available by http://" + String(host_name) + ".local/";
  printMessage(logString, true);

mqtt_tls          = true;
mqtt_auth         = true;
  if (mqtt_tls) {
    mqttClient.setClient(espSecureClient);

    // Synchronize time useing SNTP. This is necessary to verify that
    // the TLS certificates offered by the server are currently valid.
    logString = "Setting time using SNTP";
    printMessage(logString, true);
  
    configTime(8 * 3600, 0, ntp_server1, ntp_server2);
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
      delay(500);
      Serial.print(".");
      now = time(nullptr);
    }
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    logString = "Time: " + String(asctime(&timeinfo));
    printMessage(logString, true);
  
    // Load root certificate in DER format into WiFiClientSecure object
    bool result = espSecureClient.setCACert_P(caCert, caCertLen);
    if (!result) {
      logString = "Failed to load root CA certificate! Cannot continue.";
      printMessage(logString, true);
      if (use_syslog) {
        syslog.log(LOG_INFO, logString);
      }
      while (true) {
        yield();
      }
    }
  } else {
    mqttClient.setClient(espClient);
  }


  /// The MQTT baseTopic
  baseTopic = String(mqtt_name);
  baseTopic = String("homie/" + baseTopic);
  baseTopic = String(baseTopic + '/');

  app_name = "MQTT";
  logString = String("topic: " + baseTopic);
  printMessage(logString, true);
  if (use_syslog) {
    syslog.appName(app_name);
    syslog.log(LOG_INFO, logString);
  }


  logString = "server: " + String(mqtt_server) + ':' + String(mqtt_port);
  printMessage(logString, true);
  if (use_syslog) {
    syslog.log(LOG_INFO, logString);
  }

  mqttClient.setServer(mqtt_server, atoi( mqtt_port ));
  mqttClient.setCallback(mqttCallback);

  mqttConnect();
  mqttClient.loop();  // Check for incoming messages


  currentTime = millis();
  cloopTime = currentTime;

// #################### sensorSetup() is defined in each sensor file as appropriate
  sensorSetup();


  printConfig();


  app_name = "SYS";
  logString = "Startup complete.";
  printMessage(logString, true);
  if (use_syslog) {
    syslog.appName(app_name);
    syslog.log(LOG_INFO, logString);
  }


  printSystem();

  // The delays are needed to give time for each syslog packet to be sent. Otherwise a few of the later ones can end up being lost
  logString = "Flash size: " + String(realSize/1048576.0) + " MB";
  printMessage(logString, true);
  if (use_syslog) {
    syslog.log(LOG_INFO, logString);
    delay(1);
  }
  logString = "Flash size config: " + String(ideSize/1048576.0) + " MB";
  printMessage(logString, true);
  if (use_syslog) {
    syslog.log(LOG_INFO, logString);
    delay(1);
  }
  logString = "Program Size: " + String(ESP.getSketchSize() / 1024) + " kB";
  printMessage(logString, true);
  if (use_syslog) {
    syslog.log(LOG_INFO, logString);
    delay(1);
  }
  logString = "Free Program Space: " + String(ESP.getFreeSketchSpace() / 1024) + " kB";
  printMessage(logString, true);
  if (use_syslog) {
    syslog.log(LOG_INFO, logString);
    delay(1);
  }
  logString = "Free Memory: " + String(ESP.getFreeHeap() / 1024) + " kB";
  printMessage(logString, true);
  if (use_syslog) {
    syslog.log(LOG_INFO, logString);
    delay(1);
  }
  logString = "ESP Chip Id: " + String(ESP.getChipId());
  printMessage(logString, true);
  if (use_syslog) {
    syslog.log(LOG_INFO, logString);
    delay(1);
  }
  logString = "Flash Chip Id: " + String(ESP.getFlashChipId());
  printMessage(logString, true);
  if (use_syslog) {
    syslog.log(LOG_INFO, logString);
    delay(1);
  }


#ifdef DEBUG
  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
#endif
}


////////////////////////////////////// main loop ///////////////////////////////////////

void loop() {

  httpServer.handleClient();

  if (lock && abs(millis() - logincld) > 300000) {
    lock = false;
    trycount = 0;
    logincld = millis(); // After 5 minutes is passed unlock the system
  }
  if (!lock && abs(millis() - logincld) > 60000) {
    trycount = 0;
    logincld = millis();
    // After minute is passed without bad entries, reset trycount
  }
  if (abs(millis() - tempign) > 120000) {
    gencookie();
    tempign = millis();
    // if there is no activity from loged on user, generate a new cookie. This is more secure than adding expiry to the cookie header
  } 

#ifdef USESSD1306
  if ( digitalRead(CONFIG_PIN) == LOW ) {
    if ( millis() > (prevDisplay + (displaySleep * 1000) ) ) {  // if the display is sleeping redraw it.
      display.setTextSize(1);
      display.clearDisplay();
      for (byte y=0; y<(ROWS-1); y++) {
        display.setCursor(0, y*8);
        display.print(displayArray[y]);
      }
      display.display();
    }
    prevDisplay = millis();
  }
  if ( millis() > (prevDisplay + (displaySleep * 1000) ) ) {
    display.clearDisplay();
    display.display();
  }
#endif


  bool reconnected = false;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(250);
    reconnected = true;
  }
  if (reconnected == true) {
    app_name = "WIFI";
    logString = "Connected";
    printMessage(logString, true);
    if (use_syslog) {
      syslog.appName(app_name);
      syslog.log(LOG_INFO, logString);
    }
  }


  currentTime = millis();
  if (!mqttClient.connected()) {
    if ( currentTime >= (mqttConnectTime + 1000 ) ) {
      mqttConnectTime = currentTime;             // Updates mqttConnectTime
      mqttConnect();
    }
  }


  mqttClient.loop();  // Check for incoming messages



  ////////////////////////////////////// calculate data ///////////////////////////////////////
  currentTime = millis();
  calcData();



  ////////////////////////////////////// send data ///////////////////////////////////////
  // Every mqtt_interval seconds send the current data to the MQTT broker
  currentTime = millis();
  if ( currentTime >= (mqttTime + (1000 * mqtt_interval ) ) ) {
    mqttTime = currentTime;                      // Updates mqttTime
    sendData();

    /// send uptime and signal
    unsigned long uptime = millis();
    uptime = uptime / 1000;
    mqttSend(String("$uptime"), String(uptime), true);

    int signal = WiFi.RSSI();
    mqttSend(String("$signal"), String(signal), true);
  }


  ////////////////////////////////////// check the watchdog ///////////////////////////////////////
  currentTime = millis();
  if ( currentTime >= (watchdog + (1000 * mqtt_watchdog ) ) ) {
    logString = "No MQTT data in " + String(mqtt_watchdog) + " seconds. Rebooting.";
    printMessage(logString, true);

    delay(3000);
    //reboot and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }
}
