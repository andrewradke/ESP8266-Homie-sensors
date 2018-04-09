#include <FS.h>                   // this needs to be first, or it all crashes and burns...

/*
   FWTYPE:
   1  esp8266-temperature-sensor
   2  esp8266-flow-counter
   3  esp8266-switch
   4  esp8266-depth-sensor
   5  esp8266-pressure-sensors
   6  esp8266-loadcell
   7  esp8266-watermeter
   8  esp8266-bme280
   9  esp8266-sht31
   10 esp8266-air
   11 esp8266-weathervane
   12 esp8266-mq135
*/

/********************  IMPORTANT NOTE for ADS1115  ********************
   Note: change ADS1115_CONVERSIONDELAY from 8 to 9 in libraries/Adafruit_ADS1X15/Adafruit_ADS1015.h
   If this isn't done the value is read before it's converted and you will get the previous pin's reading
  #define ADS1115_CONVERSIONDELAY         (9)
 **********************************************************************/

// Much of the HTTP authentication code is based on brzi's work published at https://www.hackster.io/brzi/esp8266-advanced-login-security-748560


#define FWTYPE     12
#define FWVERSION  "0.9.16"
#define FWPASSWORD "esp8266."
//#define USESSD1306                // SSD1306 OLED display


//#define DEBUG
#define SERIALSPEED 74880

//#define LED_BUILTIN 2

#include <ESP8266WiFi.h>          // ESP8266 Core WiFi Library

#include <ESP8266WebServer.h>     // Local WebServer used to serve the configuration portal
//#include <ESP8266WebServerSecure.h> // At this stage loading the key and cert from a file doesn't work, it's too memory intensive and one page handler can't server both HTTP and HTTPS (this can be handled to some degree except for file uploads).
#include "StreamString.h"         // Need for web server firmware update
#include <ESP8266mDNS.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#include <ArduinoJson.h>          // https://github.com/bblanchon/ArduinoJson
#include <PubSubClient.h>         // http://pubsubclient.knolleary.net/


// ########### String constants ############
#include "String_Consts.h"



// Local DNS Server used for redirecting all requests to the configuration portal
#include <DNSServer.h>
DNSServer dnsServer;
const byte DNS_PORT          = 53;
bool  inConfigAP             = false;
bool  connectNewWifi         = false;
const uint16_t configPortalTimeout = 300;   // 5 minute timeout on config portal before giving up and rebooting


// Accurate time is needed to be able to verify security certificates
#include <time.h>
char     ntp_server1[41];
char     ntp_server2[41];

#include <WiFiClientSecure.h>
bool tlsOkay = false;


WiFiClientSecure espSecureClient;
WiFiClient       espClient;


#include <Syslog.h>               // https://github.com/arcao/Syslog
#include <WiFiUdp.h>
bool     use_syslog        = false;
char     host_name[21]     = "";
char     syslog_server[41];
uint16_t loglevel          = LOG_NOTICE;
WiFiUDP  udpClient;
Syslog   syslog(udpClient, SYSLOG_PROTO_IETF);


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
char mqtt_server[41];
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
PubSubClient mqttClient;


/// The HTTP server
ESP8266WebServer       httpServer(80);
//ESP8266WebServerSecure httpsServer(443);

String httpData;
bool   lock = false;                // This bool is used to control device lockout 
String httpUser;
String httpPasswd;
bool   httpLoggedin  = false;
unsigned long logincld = millis(), reqmillis = millis(), tempign = millis(); // First 2 timers are for lockout and last one is inactivity timer
uint8_t trycount = 0;                                                        // trycount will be our buffer for remembering how many failed login attempts there have been


bool rebootRequired = false;


bool      use_staticip   = false;
IPAddress ip(192, 168, 0, 99);
IPAddress dns_ip(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress gateway(192, 168, 0, 1);


String ssid  = "";
String psk   = "";
String _ssid = "";  // Used for when we are trying to connect to a new WiFi network
String _psk  = "";  // Used for when we are trying to connect to a new WiFi network


unsigned long currentTime     = 0;
unsigned long cloopTime       = 0;
unsigned long mqttTime        = 0;
unsigned long mqttConnectTime = 0;


String tmpString;
String logString;     // This can be used by MQTT logging as well


bool   configured        = false;
bool   configLoaded      = false;
String configfilename    = "/config.json";
String CAcertfilename    = "/cacert.der";
String HTTPScertfilename = "/httpscert.der";
String HTTPSkeyfilename  = "/httpskey.der";
bool   ca_cert_okay      = false;
//bool   https_okay        = false;


////////////////////////////////////// setup ///////////////////////////////////////

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
                                    // but actually the LED is on; this is because
                                    // it is acive low on the ESP-01)

#ifdef DEBUG
  tmpString = String(fwname);
  tmpString = String(tmpString + "-DEBUG");
  tmpString.toCharArray(fwname, 40);
#endif

  Serial.begin(SERIALSPEED);        // Open serial monitor at SERIALSPEED baud to see results.
  delay(200);                       // Give the serial port time to setup


  pinMode(CONFIG_PIN, INPUT_PULLUP);


// Load all the defaults into memory
  memcpy( ntp_server1,   default_ntp_server1,   41);
  memcpy( ntp_server2,   default_ntp_server2,   41);
  memcpy( syslog_server, default_syslog_server, 41);
  memcpy( mqtt_server,   default_mqtt_server,   41);
  memcpy( mqtt_name,     fwname,                21);
  strcpy(host_name, mqtt_name);

  httpUser   = String(FPSTR(default_httpUser));
  httpPasswd = String(FPSTR(default_httpPasswd));


#ifdef USESSD1306
  Serial.print( F("Configuring OLED..."));
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


  logString = String(fwname) + " v" + FWVERSION + F(" booting.");
  printMessage(app_name_sys, logString, true);

  logString = String(F("Connecting to '")) + String(WiFi.SSID()) + "'";
  printMessage(app_name_wifi, logString, true);


  uint32_t realSize = ESP.getFlashChipRealSize();
  uint32_t ideSize  = ESP.getFlashChipSize();
  if (ideSize != realSize) {
    logString = String(F("Flash size config wrong. IDE: ")) + String(ideSize/1048576.0) + F(" MB, real: ") + String(realSize/1048576.0) + " MB";
    printMessage(app_name_sys, logString, true);

    if (ideSize >= realSize) {
      logString = F("\nFlash too small!\nCANNOT BOOT\n");
      printMessage(app_name_sys, logString, true);
      bool ledState = 0;
      while (1) {
        digitalWrite(LED_BUILTIN, ledState);
        ledState = 1 - ledState;
        delay(1000);
      }      
    }
  }


  loadConfig();

  logString = String(F("MAC address: ")) + String(WiFi.macAddress());
  printMessage(app_name_wifi, logString, true);

#ifdef DEBUG
  dmesg();
  Serial.println("Saved WiFi credentials");
  dmesg();
  Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
  dmesg();
  Serial.printf("PSK:  %s\n", WiFi.psk().c_str());
#endif


  if ( digitalRead(CONFIG_PIN) == HIGH ) {  // If the config pin IS grounded then skip this
    // Back off for between 0 and 1 seconds before starting Wifi
    // This reduces the sudden current draw when too many sensors start at once or lots of data packets at exactly the same time
    long startupDelay = random(1000);
    logString = String(F("Startup random delay: ")) + String(startupDelay);
    printMessage(app_name_sys, logString, true);

    delay( startupDelay );
  }


  // This will be available in the setup AP as well
  logString = F("Starting web server");
  printMessage(app_name_http, logString, true);
  httpSetup();
   // These 3 lines tell esp to collect User-Agent and Cookie in http header when request is made 
  const char * headerkeys[] = {"User-Agent","Cookie"} ;
  size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
  httpServer.collectHeaders(headerkeys, headerkeyssize );
  httpServer.begin();


  // is configuration portal requested?
  if ( ( digitalRead(CONFIG_PIN) == LOW ) || (WiFi.SSID() == "") || (! configLoaded) ) {
    if ( digitalRead(CONFIG_PIN) == LOW )
      logString = F("Config PIN pressed.");
    if (WiFi.SSID() == "")
      logString = F("No saved SSID.");
    if (! configLoaded)
      logString = F("Config file failed to load.");
    logString += F(" Resetting configuration.");
    printMessage(app_name_cfg, logString, true);

    while ( digitalRead(CONFIG_PIN) == LOW )
      yield();
    uint16_t config_length = millis();

    if (config_length >= 10000) {
      logString = F("Config pin pressed for  more than 10 seconds. Performing factory default reset.");
      printMessage(app_name_cfg, logString, true);

      if (SPIFFS.begin()) {
        SPIFFS.format();
      }
      WiFi.persistent(true);  // Discard old WiFi credentials, i.e. erase the WiFi credentials
      WiFi.disconnect();      // Disconnect

      logString = F("Done.");
      printMessage(app_name_cfg, logString, true);

      bool ledState = 0;
      for (byte i=0;i<20;i++) {
        digitalWrite(LED_BUILTIN, ledState);
        ledState = 1 - ledState;
        delay(500);
      }
      delay(1000);
      //reboot and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(5000);
    }

    // Without this disconnect() setting mode to WIFI_AP_STA with corrupted or incorrect credentials can sometimes cause
    // the ESP8266 to get stuck and have the wdt constantly reboot before you can correct the problem. The persitant(false)
    // tells it to retain the saved credentials (i.e. don't erase anything)
    WiFi.persistent(false);  // KEEP old WiFi credentials
    WiFi.disconnect();       // but disconnect
    delay(100);
    WiFi.mode(WIFI_AP_STA);
    logString = F("Starting AP as ");
    logString += String(fwname);
    printMessage(app_name_wifi, logString, true);
    WiFi.softAP(fwname, FWPASSWORD);

    connectNewWifi = false;

    delay(500); // Without delay the IP address may be blank
#ifdef DEBUG
    dmesg();
    Serial.print(F("AP"));
    Serial.print(str_cfg_ip_address);
    Serial.print(F(": "));
    Serial.println(WiFi.softAPIP());
#endif

    /* Setup the DNS server redirecting all the domains to the apIP */
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

    inConfigAP     = true;    // We don't need to ask for a password on the webpage
    connectNewWifi = false;

    unsigned long configPortalStart = millis();
    while (true) {
      if ( millis() >= (configPortalStart + (1000 * configPortalTimeout ) ) ) {
        logString = String(F("Config portal timeout reached. ")) + str_rebooting;
        printMessage(app_name_sys, logString, true);
        delay(500);
        //reboot and try again, or maybe put it to deep sleep
        ESP.restart();
        delay(5000);
      }
      dnsServer.processNextRequest();
      httpServer.handleClient();
      if (connectNewWifi) {
        saveConfig(); // Save a config file, with the defaults if nothing else has been set.                                                                                            
        delay(100);
        if (newWiFiCredentials()) {
          break;
        }
      }
      yield();
    }
    inConfigAP     = true;
  } else {
    WiFi.begin();
  }

  WiFi.mode(WIFI_STA);

  ssid = WiFi.SSID();
  psk  = WiFi.psk();

  if (use_staticip) {
    WiFi.config(ip, gateway, subnet, dns_ip);
  }


  if (use_syslog) {
    setupSyslog();
  }
  logString = str_ipv4 + str_space + cfg_configured;
  logMessage(app_name_sys, logString, true);

/*
 *    If the serial speed is greater than 57600 baud sometimes the IP address will not be applied
 *       https://github.com/esp8266/Arduino/issues/128
 *    It appears to set the IP but then it uses a DHCP address instead. Putting enough data through the connection,
 *    such as a syslog message and a DNS lookup, seems to be enough to trigger the behaviour after which setting 
 *    the IP a second time works properly.
*/
  if (use_staticip) {
    IPAddress tmpip;
    tmpString = IPtoString(dns_ip);
    WiFi.hostByName(tmpString.c_str(), tmpip);
    while (IPtoString(WiFi.localIP()) != IPtoString(ip) ) {
      Serial.println(str_dot);
      WiFi.config(ip, gateway, subnet, dns_ip);
    }
  }



  //if you get here you have connected to the WiFi
  logString = str_connected;
  logMessage(app_name_wifi, logString, true);

  logString = String(fwname) + " v" + FWVERSION;
  logMessage(app_name_sys, logString, false);

  if (use_staticip) {
    logString = str_static;
  } else {
    waitForDHCPLease();
    logString = str_dhcp;
  }
  logString += str_space + str_cfg_ip_address + str_colon + IPtoString(WiFi.localIP());
  logMessage(app_name_net, logString, true);
  logString = str_cfg_subnet + str_colon + IPtoString(WiFi.subnetMask());
  logMessage(app_name_net, logString, false);
  logString = str_cfg_gateway + str_colon + IPtoString(WiFi.gatewayIP());
  logMessage(app_name_net, logString, false);

  if (use_staticip) {
    logString = str_cfg_dns_server + str_colon + IPtoString(dns_ip);
    logMessage(app_name_net, logString, false);
  }

/*
  bool https_files_okay = true;
  File certfile, keyfile;

  if ( SPIFFS.exists(HTTPScertfilename) ) {
    certfile = SPIFFS.open(HTTPScertfilename, "r");
    if (certfile) {
      logString = F("Opened HTTPS certificate file.");
      logMessage(app_name_http, logString, false);
    } else {
      logString = F("Failed to open HTTPS certificate! HTTPS disabled.");
      logMessage(app_name_http, logString, false);
      https_files_okay = false;
    }
  } else {
    logString = F("HTTPS certificate doesn't exist! HTTPS disabled.");
    logMessage(app_name_http, logString, false);
    https_files_okay = false;
  }

  if ( SPIFFS.exists(HTTPSkeyfilename) ) {
    keyfile = SPIFFS.open(HTTPSkeyfilename, "r");
    if (keyfile) {
      logString = F("Opened HTTPS key file.");
      logMessage(app_name_http, logString, false);
    } else {
      logString = F("Failed to open HTTPS key! HTTPS disabled.");
      logMessage(app_name_http, logString, false);
      https_files_okay = false;
    }
  } else {
    logString = F("HTTPS key doesn't exist! HTTPS disabled.");
    logMessage(app_name_http, logString, false);
    https_files_okay = false;
  }

  if (https_files_okay) {
    uint8_t x509[certfile.size()];
    uint8_t rsakey[keyfile.size()];
    certfile.read((uint8_t *)x509,  certfile.size());
    keyfile.read((uint8_t *)rsakey, keyfile.size());
    httpsServer.setServerKeyAndCert(rsakey, sizeof(rsakey), x509, sizeof(x509));

    httpsServer.collectHeaders(headerkeys, headerkeyssize );
    httpsServer.begin();
    MDNS.addService("https", "tcp", 443);
    logString = String(F("HTTPS server available by https://")) + String(host_name) + F(".local/");
    logMessage(app_name_http, logString, false);
    https_okay = true;
  } else {
    logString = F("Failed to start HTTPS server using supplied certificate and key files! HTTPS disabled.");
    logMessage(app_name_http, logString, false);
  }
  if (certfile)
    certfile.close();
  if (keyfile)
    keyfile.close();
*/

  MDNS.begin(host_name);
  MDNS.addService("http", "tcp", 80);
  logString = String(F("Web server available by http://")) + String(host_name) + F(".local/");
  logMessage(app_name_http, logString, true);

  if (mqtt_tls) {
    mqttClient.setClient(espSecureClient);

    // Synchronize time useing SNTP. This is necessary to verify that
    // the TLS certificates offered by the server are currently valid.
    logString = F("Setting time using SNTP");
    logMessage(app_name_sys, logString, false);

    configTime(8 * 3600, 0, ntp_server1, ntp_server2);
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
      delay(500);
      Serial.print(str_dot);
      now = time(nullptr);
    }
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    logString = String(asctime(&timeinfo));
    logString.trim();
    logMessage(app_name_sys, logString, true);

    if ( SPIFFS.exists(CAcertfilename) ) {
      File CAcertfile = SPIFFS.open(CAcertfilename, "r");
      if (CAcertfile) {
        logString = F("Opened CA certificate file.");
        logMessage(app_name_mqtt, logString, false);

        if (espSecureClient.loadCACert(CAcertfile, CAcertfile.size()) ) {
          ca_cert_okay = true;
        } else {
          logString = F("Failed to load root CA certificate! MQTT disabled.");
          logMessage(app_name_mqtt, logString, false);
        }
        CAcertfile.close();
      } else {
        logString = F("Failed to open CA certificate! MQTT disabled.");
        logMessage(app_name_mqtt, logString, false);
      }
    } else {
      logString = F("CA certificate doesn't exist! MQTT disabled.");
      logMessage(app_name_mqtt, logString, false);
    }
  } else {
    mqttClient.setClient(espClient);
  }


  /// The MQTT baseTopic
  baseTopic = String(mqtt_name);
  baseTopic = String("homie/" + baseTopic);
  baseTopic = String(baseTopic + '/');

  logString = String("topic: " + baseTopic);
  logMessage(app_name_mqtt, logString, false);

  logString = "server: " + String(mqtt_server) + ':' + String(mqtt_port);
  logMessage(app_name_mqtt, logString, false);

  mqttClient.setServer(mqtt_server, atoi( mqtt_port ));
  mqttClient.setCallback(mqttCallback);

  mqttConnect();
  mqttClient.loop();  // Check for incoming messages


  currentTime = millis();
  cloopTime = currentTime;

// #################### sensorSetup() is defined in each sensor file as appropriate
  sensorSetup();


  mqtt_send_systeminfo();

  logString = F("Startup complete.");
  logMessage(app_name_sys, logString, true);


#ifdef DEBUG
  logString = String(F("Flash size: ")) + String(realSize/1048576.0) + " MB";
  logMessage(app_name_sys, logString, false);
  logString = String(F("Flash size config: ")) + String(ideSize/1048576.0) + " MB";
  logMessage(app_name_sys, logString, false);
  logString = String(F("Program Size: ")) + String(ESP.getSketchSize() / 1024) + " kB";
  logMessage(app_name_sys, logString, false);
  logString = String(F("Free Program Space: ")) + String(ESP.getFreeSketchSpace() / 1024) + " kB";
  logMessage(app_name_sys, logString, false);

  logString = String(F("Free Memory: ")) + String(ESP.getFreeHeap() / 1024) + " kB";
  logMessage(app_name_sys, logString, false);

  FSInfo fs_info;
  SPIFFS.info(fs_info);
  logString = String(F("Filesystem size:")) + String(fs_info.totalBytes / 1024) + " kB";
  logMessage(app_name_sys, logString, false);
  logString = String(F("Filesystem free:")) + String((fs_info.totalBytes - fs_info.usedBytes) / 1024) + " kB";
  logMessage(app_name_sys, logString, false);

  logString = String(F("ESP Chip Id: ")) + String(ESP.getChipId());
  logMessage(app_name_sys, logString, false);
  logString = String(F("Flash Chip Id: ")) + String(ESP.getFlashChipId());
  logMessage(app_name_sys, logString, false);
#endif


#ifdef DEBUG
  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
#endif
}


////////////////////////////////////// main loop ///////////////////////////////////////

void loop() {

  httpServer.handleClient();
//  if (https_okay)
//    httpsServer.handleClient();
  if (connectNewWifi) {
    newWiFiCredentials();  // Attempt to connect to new WiFi network
  }

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
    httpLoggedin = false;
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
    checkWatchdog();      // Let the watchdog reboot if we've been disconnected too long
    reconnected = true;   // Records that we have reconnected
  }
  if (reconnected == true) {
    logString = str_connected;
    logMessage(app_name_wifi, logString, true);

    if (!use_staticip) {
      waitForDHCPLease();
    }

    logString = str_connected;
    logMessage(app_name_net, logString, true);
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
  checkWatchdog();
}
