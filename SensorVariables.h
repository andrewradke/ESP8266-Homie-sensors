#ifndef FWTYPE

#error FWTYPE not defined


/*
// ************* esp8266-template-sensor *************
// Unused. Template ONLY.
#elif FWTYPE == 0

/*
char fwname[40] =    "esp8266-dummy-sensor";
char nodes[100] =    "sensor:number";

float    sensor_value = 42.0;

#define WMADDCONFIG \
  char config_itemStr[8]; \
  dtostrf(config_item, 6, 2, config_itemStr); \
  WiFiManagerParameter pressure_text("Config item:"); \
  wifiManager.addParameter(&pressure_text); \
  WiFiManagerParameter custom_config_item("config_item", "Config item", config_itemStr, 8); \
  wifiManager.addParameter(&custom_config_item);

#define WMGETCONFIG \
    config_item     = atof(custom_config_item.getValue());
*/


// ************* esp8266-temperature-sensor *************
#elif FWTYPE == 1

char fwname[40] =    "esp8266-temperature-sensor";
#define ONEWIRE_PIN  2          // WeMos D1 Mini D4
char nodes[100] =    "temperature:celsius";
#include <OneWire.h>
OneWire ds(ONEWIRE_PIN);        // Setup of the 1-wire bus
float   temperature  = 1000;


// ************* esp8266-flow-counter *************
#elif FWTYPE == 2

char fwname[40] =    "esp8266-flow-counter";
// #define PULSENUM  7.5        // YF-S201C
#define PULSENUM     1.0        // YF-G1
#define PULSE_PIN    12         // WeMos D1 Mini D6
char nodes[100] =    "counter:counter,litres:litres,lph:lph";
volatile long flow_counter = 0; // Measures flow meter pulses
unsigned long flow_hz      = 0; // Measures last seconds flow
unsigned int  l_hour;           // Calculated litres/hour
float         litres       = 0;
const float   flowrate     = 60 / PULSENUM;
const float   litrerate    = 60 * PULSENUM;


// ************* esp8266-switch *************
#elif FWTYPE == 3

char fwname[40] =    "esp8266-switch";
#define SWITCH_PIN   12         // WeMos D1 Mini D6
char nodes[100] =    "switch:switch";
bool          switchOpen     = false;
bool          switchPrevOpen = false;


// ************* esp8266-depth-sensor *************
#elif FWTYPE == 4

char fwname[40] =    "esp8266-depth-sensor";
#define TRIGGER_PIN  5          // WeMos D1 Mini D1
#define ECHO_PIN     4          // WeMos D1 Mini D2
#define MAX_DISTANCE 400        // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
char nodes[100] =    "distance:cm,depth:cm";
#include <NewPing.h>            // https://bitbucket.org/teckel12/arduino-new-ping/downloads
#define US_ROUNDTRIP_CM 58      // 58uS per cm distance at around 20-25 C
#define ONE_PIN_ENABLED false
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
unsigned int uS = 0;

uint16_t      distance = 0;
uint16_t      depth    = 0;
uint16_t      maxdepth = 0;
byte          readings = 10;


// ************* esp8266-pressure-sensor *************
#elif FWTYPE == 5

char fwname[40] =    "esp8266-pressure-sensor";
char nodes[100] =    "analog0:pressure,analog1:pressure,analog2:pressure,analog3:pressure";
#include <Adafruit_ADS1015.h>
Adafruit_ADS1115 ads1115;
float pressureRanges[5] = {1, 1, 1, 1};

#define WMADDCONFIG \
    WiFiManagerParameter pressure_text("Pressure ranges:"); \
    wifiManager.addParameter(&pressure_text); \
    char pressureRange[8]; \
    dtostrf(pressureRanges[0], 6, 2, pressureRange); \
    WiFiManagerParameter custom_pressureRange0("A0pressure", "A0 max pressure", pressureRange, 8); \
    wifiManager.addParameter(&custom_pressureRange0); \
    dtostrf(pressureRanges[1], 6, 2, pressureRange); \
    WiFiManagerParameter custom_pressureRange1("A1pressure", "A1 max pressure", pressureRange, 8); \
    wifiManager.addParameter(&custom_pressureRange1); \
    dtostrf(pressureRanges[2], 6, 2, pressureRange); \
    WiFiManagerParameter custom_pressureRange2("A2pressure", "A2 max pressure", pressureRange, 8); \
    wifiManager.addParameter(&custom_pressureRange2); \
    dtostrf(pressureRanges[3], 6, 2, pressureRange); \
    WiFiManagerParameter custom_pressureRange3("A3pressure", "A3 max pressure", pressureRange, 8); \
    wifiManager.addParameter(&custom_pressureRange3);

#define WMGETCONFIG \
    pressureRanges[0] = atof(custom_pressureRange0.getValue()); \
    pressureRanges[1] = atof(custom_pressureRange1.getValue()); \
    pressureRanges[2] = atof(custom_pressureRange2.getValue()); \
    pressureRanges[3] = atof(custom_pressureRange3.getValue());


// ************* esp8266-loadcell *************
#elif FWTYPE == 6

char fwname[40] =    "esp8266-loadcell";
#define PIN_SCALE_DOUT 16     // WeMos D1 Mini D0
#define PIN_SCALE_SCK  14     // WeMos D1 Mini D5
#define PIN_SCALE_PDWN 12     // WeMos D1 Mini D6
char nodes[100] =    "weight:grams";
#include <HX711.h>                // https://github.com/bogde/HX711
HX711 hx711;
float         weight;
float         scale    = 440.24;
unsigned long offset   = 1296368;
byte          readings = 5;


// ************* esp8266-5v-sensorboard *************
#elif FWTYPE == 7

char fwname[40] =    "esp8266-5v-sensorboard";
char nodes[100] =    "voltage:volts,pressure:psi,counter:counter,litres:litres,lph:lph,switch:switch";

// #define PULSENUM  7.5        // YF-S201C
#define PULSENUM     1.0        // YF-G1

// D4 = GPIO2
// D7 = GPIO13
#define PULSE_PIN      2      // WeMos D1 Mini D4
#define SWITCH_PIN     12     // WeMos D1 Mini D6

#ifdef USEI2CADC
#include <Wire.h>
#include <SW_MCP3221.h>         // https://github.com/SparkysWidgets/MCP3221-Library
const byte i2cAddress = 0x4D;
const int  I2CadcVRef = 5000;    // Measured millivolts of voltage input to ADC
MCP3221 i2cADC(i2cAddress, I2CadcVRef);
#else
#define ANALOG         A0
#endif

float         voltage;
float         pressure;
float         pressureRange = 1;

volatile long flow_counter = 0; // Measures flow meter pulses
unsigned long flow_hz      = 0; // Measures last seconds flow
unsigned int  l_hour;           // Calculated litres/hour
float         litres       = 0;
const float   flowrate     = 60 / PULSENUM;
const float   litrerate    = 60 * PULSENUM;

bool          switchOpen     = false;
bool          switchPrevOpen = false;

#define WMADDCONFIG \
    char pressureRangeStr[8]; \
    dtostrf(pressureRange, 6, 2, pressureRangeStr); \
    WiFiManagerParameter pressure_text("Pressure range:"); \
    wifiManager.addParameter(&pressure_text); \
    WiFiManagerParameter custom_pressureRange("pressure", "Pressure range", pressureRangeStr, 8); \
    wifiManager.addParameter(&custom_pressureRange);

#define WMGETCONFIG \
    pressureRange = atof(custom_pressureRange.getValue());


// ************* esp8266-bme280 *************
#elif FWTYPE == 8

char fwname[40] =    "esp8266-bme280";
char nodes[100] =    "temperature:celsius,humidity:percentage,pressure:hpa,pressure-sealevel:hpa";

float    temperature  = 1000;
uint16_t humidity     = 1000;
float    dewpoint     = 1000;
float    pressure     = 0;
float    sealevel     = 0;
float    elevation    = 0;

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
// SDO is connected to GND so address is 0x76 instead of 0x77
// #define BME280_ADDRESS  (0x76) This needs to be changed in the library
Adafruit_BME280 bme; // I2C
float    temperature2 = 1000;
uint16_t humidity2    = 1000;
float    pressure2    = 0;
float    sealevel2    = 0;
bool     bme280Status = 0;
bool     bme280Error  = 0;
int      bme280ErrorT = 0;
int      bme280ErrorH = 0;
int      bme280ErrorP = 0;

#define WMADDCONFIG \
  char elevationStr[8]; \
  dtostrf(elevation, 6, 2, elevationStr); \
  WiFiManagerParameter pressure_text("Elevation:"); \
  wifiManager.addParameter(&pressure_text); \
  WiFiManagerParameter custom_elevation("elevation", "Elevation", elevationStr, 8); \
  wifiManager.addParameter(&custom_elevation);

#define WMGETCONFIG \
    elevation     = atof(custom_elevation.getValue());


// ************* esp8266-sht31 *************
#elif FWTYPE == 9

char fwname[40] =    "esp8266-sht31";
char nodes[100] =    "temperature:celsius,humidity:percentage";

float    temperature  = 1000;
float    humidity     = 1000;
float    dewpoint     = 1000;

#include <Wire.h>
#include <Adafruit_SHT31.h>

Adafruit_SHT31 sht31 = Adafruit_SHT31();

float    temperature1 = 1000;
float    humidity1    = 1000;
bool     sht31Status  = 0;
bool     sht31Error   = 0;
int      sht31ErrorT  = 0;
int      sht31ErrorH  = 0;


// ************* esp8266-air *************
#elif FWTYPE == 10

char fwname[40] =    "esp8266-air";
char nodes[100] =    "temperature:celsius,humidity:percentage,dewpoint:celsius,pressure:hpa,pressure-sealevel:hpa";

float    temperature  = 1000;
float    humidity     = 1000;
float    dewpoint     = 1000;
float    pressure     = 0;
float    sealevel     = 0;
float    elevation    = 0;

#include <Wire.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

Adafruit_SHT31 sht31 = Adafruit_SHT31();

float    temperature1 = 1000;
float    humidity1    = 1000;
bool     sht31Status  = 0;
bool     sht31Error   = 0;
int      sht31ErrorT  = 0;
int      sht31ErrorH  = 0;

// SDO is connected to GND so address is 0x76 instead of 0x77
// #define BME280_ADDRESS  (0x76) This needs to be changed in the library
Adafruit_BME280 bme; // I2C
float    temperature2 = 1000;
uint16_t humidity2    = 1000;
float    pressure2    = 0;
float    sealevel2    = 0;
bool     bme280Status = 0;
bool     bme280Error  = 0;
int      bme280ErrorT = 0;
int      bme280ErrorH = 0;
int      bme280ErrorP = 0;

#define WMADDCONFIG \
  char elevationStr[8]; \
  dtostrf(elevation, 6, 2, elevationStr); \
  WiFiManagerParameter pressure_text("Elevation:"); \
  wifiManager.addParameter(&pressure_text); \
  WiFiManagerParameter custom_elevation("elevation", "Elevation", elevationStr, 8); \
  wifiManager.addParameter(&custom_elevation);

#define WMGETCONFIG \
    elevation     = atof(custom_elevation.getValue());

// ************* esp8266-weathervane *************
#elif FWTYPE == 11

char fwname[40] =    "esp8266-weathervane";

#define RAINPULSE      0.2794        // mm per pulse
#define WINDPULSE      2.4           // kph per pulse per second

#define RAINGAUGE_PIN  12            // pin tied to the pulse output of the rain gauge
#define ANEMOMETER_PIN 14            // pin tied to the pulse output of the anemometer
#define WINDVANE_PIN   A0            // pin tied to the analog output of the wind vane

char nodes[106] =    "rain:mm,rain_counter:counter,wind_direction:degrees,wind_speed:kph,gust_direction:degrees,gust_speed:kph";

uint16_t         voltage;
volatile uint8_t wind_counter = 0;       // Measures anemometer pulses
uint8_t          dir_reading;            // Wind direction. 0=N, 1=NNE, ... 15=NNW, 16=shorted to GND, 17=shorted to 5V
float            wind_speed;             // Calculated kph
float            wind_dir;               // Calculated wind direction
float            gust_speed;             // Calculated kph
float            gust_dir;               // Calculated wind direction
volatile long    rain_counter = 0;       // Measures rain gauge pulses
float            rain_mm      = 0;       // Calculated rain

// http://wiki.wunderground.com/index.php/Educational_-_Wind_gusts_vs._gust_speed
#define          USEALLGUSTS
// The Australian Bureau of Meteorology averages their readings over 10 minutes, 600 seconds
#define          WIND_SECS 600           // Number of seconds over which to calculate the wind speed average, 
uint8_t          wind_speeds[WIND_SECS]; // Array of last two minutes of wind speeds to calculate average from
uint8_t          wind_dirs[WIND_SECS];   // Array of last two minutes of wind direction to calculate average from
uint16_t         wind_array_pos = 0;     // Current position in the array
bool             wind_array_full = false;

// ************* UNKNOWN SENSOR TYPE *************
#else
#error Unknown FWTYPE code
#endif
