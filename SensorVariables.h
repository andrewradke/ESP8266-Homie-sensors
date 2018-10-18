#ifndef FWTYPE

#error FWTYPE not defined


/*
// ************* esp8266-template-sensor *************
// Unused. Template ONLY.
#elif FWTYPE == 0

char fwname[40] =    "esp8266-dummy-sensor";
char nodes[100] =    "sensor:number";

float    sensor_value = 42.0;
#define CONFIG_PIN 13
*/


// ************* esp8266-temperature-sensor *************
#elif FWTYPE == 1

char fwname[40] =    "esp8266-temperature-sensor";
#define ONEWIRE_PIN  2          // WeMos D1 Mini D4
#define CONFIG_PIN   13         // WeMos D1 Mini D7
char nodes[100] =    "temperature:celsius";
#include <OneWire.h>
OneWire ds(ONEWIRE_PIN);        // Setup of the 1-wire bus
float   temperature  = 1000;



// ************* esp8266-flow-counter *************
#elif FWTYPE == 2

char fwname[40] =    "esp8266-flow-counter";
#define PULSE_PIN    12         // WeMos D1 Mini D6
#define CONFIG_PIN   13         // WeMos D1 Mini D7
char nodes[100] =    "counter:counter,litres:litres,lph:lph";
volatile long flow_counter = 0; // Measures flow meter pulses
unsigned long flow_hz      = 0; // Measures last seconds flow
unsigned int  l_hour;           // Calculated litres/hour
float         litres       = 0;
float         pulsesPerLitre = 1;   // YF-G1 = 1, YF-S201C = 7.5z
float         flowrate     = 60 / pulsesPerLitre;
float         litrerate    = 60 * pulsesPerLitre;



// ************* esp8266-switch *************
#elif FWTYPE == 3

char fwname[40] =    "esp8266-switch";
#define SWITCH_PIN   12         // WeMos D1 Mini D6
#define CONFIG_PIN   13         // WeMos D1 Mini D7
char nodes[100] =    "switch:switch";
bool          switchOpen     = false;
bool          switchPrevOpen = false;

const char switchOpenNoun[]   = "open";
const char switchClosedNoun[] = "closed";
const char switchNodeVerb[]   = "switch/switch";



// ************* esp8266-depth-sensor *************
#elif FWTYPE == 4

// D4 = GPIO2
// D6 = GPIO12
char fwname[40] =    "esp8266-depth-sensor";
#define TRIGGER_PIN  2          // WeMos D1 Mini D4
#define ECHO_PIN     12         // WeMos D1 Mini D6
#define CONFIG_PIN   13         // WeMos D1 Mini D7
#define MAX_DISTANCE 450        // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
char nodes[100] =    "distance:cm,depth:cm";
#include <NewPing.h>            // https://bitbucket.org/teckel12/arduino-new-ping/downloads
#define US_ROUNDTRIP_CM 58      // 58uS per cm distance at around 20-25 C
#define ONE_PIN_ENABLED false
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
unsigned int uS = 0;

uint16_t      distance = 0;
uint16_t      depth    = 450;   // The mas distane reading is around 450cm so use that as a default value until it's changed in the config page
float         maxdepth = 0;
byte          readings = 10;



// ************* esp8266-pressure-sensors *************
#elif FWTYPE == 5

char fwname[40] =    "esp8266-pressure-sensors";
char nodes[100] =    "pressure0:pressure,pressure1:pressure,pressure2:pressure,pressure3:pressure";
#define CONFIG_PIN   13         // WeMos D1 Mini D7
#include <Adafruit_ADS1015.h>
Adafruit_ADS1115 ads1115;
float pressureRanges[5] = {1, 1, 1, 1};



// ************* esp8266-loadcell *************
#elif FWTYPE == 6

char fwname[40] =    "esp8266-loadcell";
#define PIN_SCALE_DOUT 16     // WeMos D1 Mini D0
#define PIN_SCALE_SCK  14     // WeMos D1 Mini D5
#define PIN_SCALE_PDWN 12     // WeMos D1 Mini D6
#define CONFIG_PIN     13     // WeMos D1 Mini D7
char nodes[100] =    "weight:grams";
#include <HX711.h>                // https://github.com/bogde/HX711
HX711 hx711;
float         weight;
float         scale    = 440.24;
unsigned long offset   = 1296368;
byte          readings = 5;



// ************* esp8266-watermeter *************
#elif FWTYPE == 7

char fwname[40] =    "esp8266-watermeter";
char nodes[100] =    "voltage:volts,pressure:psi,counter:counter,litres:litres,lph:lph,switch:switch";

#define USEI2CADC

// D4 = GPIO2
// D7 = GPIO13
#define PULSE_PIN      12     // WeMos D1 Mini D6
#define SWITCH_PIN     14     // WeMos D1 Mini D5
#define CONFIG_PIN     13     // WeMos D1 Mini D7

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
float         pulsesPerLitre = 1;   // YF-G1 = 1, YF-S201C = 7.5
float         flowrate     = 60 / pulsesPerLitre;
float         litrerate    = 60 * pulsesPerLitre;

bool          switchOpen     = false;
bool          switchPrevOpen = false;

const char switchOpenNoun[]   = "open";
const char switchClosedNoun[] = "closed";


// ************* esp8266-bme280 *************
#elif FWTYPE == 8

char fwname[40] =    "esp8266-bme280";
char nodes[100] =    "temperature:celsius,humidity:percentage,dewpoint:celsius,pressure:hpa,pressure-sealevel:hpa";
#define CONFIG_PIN   13         // WeMos D1 Mini D7
#define I2C_ADDRESS  0x77       // 0x76 or 0x77

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
float    readTemperature = 1000;
uint16_t readHumidity    = 1000;
float    readPressure    = 0;
float    readSeaLevel    = 0;
bool     sensorStatus = 0;
bool     sensorError  = 0;
int      sensorErrorT = 0;
int      sensorErrorH = 0;
int      sensorErrorP = 0;
bool     humidity100Okay = true;



// ************* esp8266-sht31 *************
#elif FWTYPE == 9

char fwname[40] =    "esp8266-sht31";
char nodes[100] =    "temperature:celsius,humidity:percentage,dewpoint:celsius";
#define CONFIG_PIN   13         // WeMos D1 Mini D7

float    temperature  = 1000;
float    humidity     = 1000;
float    dewpoint     = 1000;

#include <Wire.h>
#include <Adafruit_SHT31.h>

Adafruit_SHT31 sht31 = Adafruit_SHT31();

float    readTemperature = 1000;
uint16_t readHumidity    = 1000;
bool     sensorStatus = 0;
bool     sensorError  = 0;
int      sensorErrorT = 0;
int      sensorErrorH = 0;
bool     humidity100Okay = true;




// ************* esp8266-air *************
#elif FWTYPE == 10

char fwname[40] =    "esp8266-air";
char nodes[100] =    "temperature:celsius,humidity:percentage,dewpoint:celsius,pressure:hpa,pressure-sealevel:hpa";
#define CONFIG_PIN   13         // WeMos D1 Mini D7

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




// ************* esp8266-weathervane *************
#elif FWTYPE == 11

char fwname[40] =    "esp8266-weathervane";

float kphPerPulse = 2.4;             // kph per pulse per second
float mmPerPulse  = 0.2794;          // mm per pulse

#define RAINGAUGE_PIN  12            // pin tied to the pulse output of the rain gauge
#define ANEMOMETER_PIN 14            // pin tied to the pulse output of the anemometer
#define WINDVANE_PIN   A0            // pin tied to the analog output of the wind vane
#define CONFIG_PIN     13            // WeMos D1 Mini D7

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



// ************* esp8266-mq135 *************
#elif FWTYPE == 12

char fwname[40] =    "esp8266-mq135";
char nodes[100] =    "co2:ppm";

#define SENSOR_PIN   A0
#define CONFIG_PIN   13         // WeMos D1 Mini D7

#define MQ135_DEFAULTPPM 399            // default ppm of CO2 for calibration
#define MQ135_SCALINGFACTOR 116.6020682 // CO2 gas value
#define MQ135_EXPONENT -2.769034857     // CO2 gas value
#define MQ135_MAXRSRO 2.428             // for CO2
#define MQ135_MINRSRO 0.358             // for CO2

uint32_t mq135_ro = 68550;              // default Ro for MQ135_DEFAULTPPM ppm of CO2
double   val      = 0;                  // variable to store the value coming from the sensor
double   valAIQ   = 0;
//double   lastAIQ  = 0;



// ************* esp8266-timer *************
#elif FWTYPE == 13

#define NUM_TIMERS 100
char fwname[40] =    "esp8266-timer";
#define CONFIG_PIN   2          // 2  = BTN1 on ElectroDragon relay

// This should be built by a loop based on the NUM_TIMERS
char nodes[100] =    "timers[]:timer";

uint16_t timers[NUM_TIMERS][3];         // 3 numbers per timer: day of week bits, minute of day, length of timer
uint8_t  relay_pin    = 12;             // Config setting will override this default (Electrodragon relay 1)
uint32_t lastTimer    = 0;              // The last *millis* a check was made for a timer
uint16_t lastMinute   = 0;              // The last *minute* that the timer check was made
bool     relay_state  = false;
uint32_t relayOffTime = 0;              // The *seconds* at which the relay needs to be turned off



// ************* esp8266-pump-controller *************
#elif FWTYPE == 14

char fwname[40] =    "esp8266-pump-controller";
char nodes[100] =    "pressure:psi,litres:litres,lph:lph,pump:power,cutoff:bool";

#define PULSE_PIN      14     // WeMos D1 Mini D5
#define RELAY_PIN      16     // WeMos D1 Mini D0
#define CONFIG_PIN     13     // WeMos D1 Mini D7
#define ANALOG         A0

#include <inttypes.h>

float         voltage;
float         pressure;
float         pressureRange  = 72.5;

volatile long flow_counter   = 0;     // Measures flow meter pulses
unsigned long flow_hz        = 0;     // Measures last seconds flow
unsigned int  l_hour;                 // Calculated litres/hour
unsigned long floopTime      = 0;
float         litres         = 0;
float         pulsesPerLitre = 1;     // YF-G1 = 1
float         flowrate       = 60 / pulsesPerLitre;
float         litrerate      = 60 * pulsesPerLitre;

int32_t       cutoffLow      = 10;    // Minimum pressure for pump to be kept on before it's considered to have lost prime
int32_t       cutoffHigh     = 60;    // Maximum pressure for pump to be kept on as a failsafe
int32_t       pressureOn     = 30;    // Low pressure at which point the pump turns on
int32_t       pressureOff    = 50;    // High pressure at which point the pump turns off
int32_t       flowMin        = 1800;  // Minimum flow in lph before turning pump off
uint32_t      pumpTimer      = 0;     // Start a timer when the pump turns on and monitor flow and pressure after this
uint8_t       pumpCheckDelay = 2;     // The number of seconds to wait after the pump has turned on before checking flow is okay

bool          pumpState      = false;
bool          pumpPrevState  = false;
bool          pumpCutoff     = false;



// ************* esp8266-tli4970 *************
#elif FWTYPE == 15

char fwname[40] =    "esp8266-tli4970";
#define PIN_TLI_CS   15     // WeMos D1 Mini D8
#define PIN_TLI_DIO  12     // WeMos D1 Mini D6
#define PIN_TLI_SCK   0     // WeMos D1 Mini D5
#define PIN_TLI_OCD 255     // WeMos D1 Mini D4
#define CONFIG_PIN   13     // WeMos D1 Mini D7
char nodes[100] =    "current:amps";
#include <Tli4970.h>              // https://github.com/Infineon/TLI4970-D050T4-Current-Sensor
Tli4970 Tli4970CurrentSensor = Tli4970();
float         current       = 0;
uint32_t      readings      = 0;
uint16_t      tli4970Errors = 0;
uint32_t      sLoopTime     = 0;
uint32_t      dLoopTime     = 0;
bool          acLoad        = false;



// ************* esp8266-pressure_depth *************
#elif FWTYPE == 16

char fwname[40] =    "esp8266-pressure_depth";
#define CONFIG_PIN   13         // WeMos D1 Mini D7
char nodes[100] =    "voltage:volts,pressure:psi,depth:metres";
#define ANALOG       A0
float         voltage;
float         pressure;
float         depth;
float         pressureRange = 15; // 15 psi is a good default pressure for reading most tank depths
float         depthOffset   = 0;  // Allow readings to be moved up or down depnding on the level the sensor is sitting in the water



// ************* UNKNOWN SENSOR TYPE *************
#else
#error Unknown FWTYPE code
#endif
