///// Default values

static const char default_ntp_server1[41]   PROGMEM = "0.pool.ntp.org";
static const char default_ntp_server2[41]   PROGMEM = "1.pool.ntp.org";
static const char default_syslog_server[41] PROGMEM = "syslog.local";
static const char default_mqtt_server[41]   PROGMEM = "mqtt.local";
static const char default_httpUser[]        PROGMEM = "admin";
static const char default_httpPasswd[]      PROGMEM = "esp8266";


///// strings used in various places

const String app_name_sys     = "SYS";
const String app_name_net     = "NET";
const String app_name_cfg     = "CFG";
const String app_name_wifi    = "WIFI";
const String app_name_mqtt    = "MQTT";
const String app_name_http    = "HTTP";
const String app_name_sensors = "Sensors";


///// strings used for config

static const char cfg_use_staticip[17]  PROGMEM = "use_staticip";
static const char cfg_ip_address[17]    PROGMEM = "ip_address";
static const char cfg_subnet[17]        PROGMEM = "subnet";
static const char cfg_gateway[17]       PROGMEM = "gateway";
static const char cfg_dns_server[17]    PROGMEM = "dns_server";
static const char cfg_ntp_server1[17]   PROGMEM = "ntp_server1";
static const char cfg_ntp_server2[17]   PROGMEM = "ntp_server2";
static const char cfg_mqtt_server[17]   PROGMEM = "mqtt_server";
static const char cfg_mqtt_port[17]     PROGMEM = "mqtt_port";
static const char cfg_mqtt_name[17]     PROGMEM = "mqtt_name";
static const char cfg_mqtt_tls[17]      PROGMEM = "mqtt_tls";
static const char cfg_mqtt_auth[17]     PROGMEM = "mqtt_auth";
static const char cfg_mqtt_user[17]     PROGMEM = "mqtt_user";
static const char cfg_mqtt_passwd[17]   PROGMEM = "mqtt_passwd";
static const char cfg_mqtt_interval[17] PROGMEM = "mqtt_interval";
static const char cfg_mqtt_watchdog[17] PROGMEM = "mqtt_watchdog";
static const char cfg_use_syslog[17]    PROGMEM = "use_syslog";
static const char cfg_host_name[17]     PROGMEM = "host_name";
static const char cfg_syslog_server[17] PROGMEM = "syslog_server";
static const char cfg_httpUser[17]      PROGMEM = "httpUser";
static const char cfg_httpPasswd[17]    PROGMEM = "httpPasswd";
static const char cfg_configured[17]    PROGMEM = "configured";


static const char mqttstr_log[]       PROGMEM = "$log";
static const char mqttstr_online[]    PROGMEM = "$online";
static const char mqttstr_fwname[]    PROGMEM = "$fwname";
static const char mqttstr_fwversion[] PROGMEM = "$fwversion";
static const char mqttstr_name[]      PROGMEM = "$name";
static const char mqttstr_nodes[]     PROGMEM = "$nodes";
static const char mqttstr_localip[]   PROGMEM = "$localip";
static const char mqttstr_config[]    PROGMEM = "$config/";
static const char mqttstr_system[]    PROGMEM = "$system/";
static const char mqttstr_ota[]       PROGMEM = "$ota/";

static const char str_command[]    PROGMEM = "command";
static const char str_error[]      PROGMEM = "error";
static const char str_true[]       PROGMEM = "true";
static const char str_false[]      PROGMEM = "false";
static const char str_nbsp[]       PROGMEM = "&nbsp;";

const String httpFooter = "</body></html>";
const String tableStart = "<table>";
const String tableEnd   = "</table>";
const String trStart    = "<tr><td>";
const String trEnd      = "</td></tr>";
const String tdBreak    = "</td><td>";
const String thStart    = "<tr><th>";
const String thEnd      = "</th></tr>";
const String thBreak    = "</th><th>";

static const char str_firmware_update[]       PROGMEM = "Firmware update: ";

