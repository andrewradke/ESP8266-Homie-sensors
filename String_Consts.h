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

const String cfg_use_staticip  = "use_staticip";
const String cfg_ip_address    = "ip_address";
const String cfg_subnet        = "subnet";
const String cfg_gateway       = "gateway";
const String cfg_dns_server    = "dns_server";
const String cfg_ntp_server1   = "ntp_server1";
const String cfg_ntp_server2   = "ntp_server2";
const String cfg_mqtt_server   = "mqtt_server";
const String cfg_mqtt_port     = "mqtt_port";
const String cfg_mqtt_name     = "mqtt_name";
const String cfg_mqtt_tls      = "mqtt_tls";
const String cfg_mqtt_auth     = "mqtt_auth";
const String cfg_mqtt_user     = "mqtt_user";
const String cfg_mqtt_passwd   = "mqtt_passwd";
const String cfg_mqtt_interval = "mqtt_interval";
const String cfg_mqtt_watchdog = "mqtt_watchdog";
const String cfg_use_syslog    = "use_syslog";
const String cfg_host_name     = "host_name";
const String cfg_syslog_server = "syslog_server";
const String cfg_httpUser      = "httpUser";
const String cfg_httpPasswd    = "httpPasswd";
const String cfg_configured    = "configured";


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

const String str_on         = "on";
const String str_true       = "true";
const String str_false      = "false";
const String str_from       = " from ";
const String str_to         = " to ";
const String str_nbsp       = "&nbsp;";
const String str_succeeded  = "succeeded";
const String str_failed     = "failed";
const String str_text       = "text";
const String str_password   = "password";
const String str_static     = "Static";
const String str_dhcp       = "DHCP";
const String str_space      = " ";
const String str_dot        = ".";
const String str_colon      = ": ";
const String str_ipv4       = "IPv4";

const String httpFooter = "</body></html>";
const String tableStart = "<table>";
const String tableEnd   = "</table>";
const String trStart    = "<tr><td>";
const String trEnd      = "</td></tr>";
const String tdBreak    = "</td><td>";
const String thStart    = "<tr><th>";
const String thEnd      = "</th></tr>";
const String thBreak    = "</th><th>";

const String str_firmware_update = "Firmware update: ";
const String str_rebooting       = "Rebooting.";
const String str_connected       = "Connected";

const String str_cfg_changed     = "Config changed for ";
const String str_cfg_overridden  = "Config overridden for ";

const String str_cfg_staticip      = "staticip";
const String str_cfg_ip_address    = "IP address";
const String str_cfg_subnet        = "Subnet mask";
const String str_cfg_gateway       = "Gateway";
const String str_cfg_dns_server    = "DNS server";
const String str_cfg_ntp_server1   = "NTP server 1";
const String str_cfg_ntp_server2   = "NTP server 2";
const String str_cfg_mqtt_server   = "MQTT server";
const String str_cfg_mqtt_port     = "MQTT port";
const String str_cfg_mqtt_name     = "MQTT name";
const String str_cfg_mqtt_tls      = "MQTT TLS";
const String str_cfg_mqtt_auth     = "MQTT authentication";
const String str_cfg_mqtt_user     = "MQTT user";
const String str_cfg_mqtt_passwd   = "MQTT password";
const String str_cfg_mqtt_interval = "MQTT interval";
const String str_cfg_mqtt_watchdog = "MQTT watchdog timer";
const String str_cfg_use_syslog    = "Use syslog";
const String str_cfg_host_name     = "hostname";
const String str_cfg_syslog_server = "Syslog server";
