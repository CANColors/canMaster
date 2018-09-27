/* ========================================================================== */
/*                                                                            */
/*   Filename.c                                                               */
/*   (c) 2012 Author                                                          */
/*                                                                            */
/*   Description                                                              */
/*                                                                            */
/* ========================================================================== */

#define MQTT_REQUEST_TOPIC      "request"
#define MQTT_HEARTBEAT_TOPIC    "heartbeat"
#define MQTT_RESPONSE_TOPIC     "response"
 // #define HTTP_RESPONSE_DATA_PAGE     "echo"


#define NET_TRANSMIT_TIMESLOT  100
#define TIME_HEARTBEAT        5000
#define RESPONSE_DELAY        1
#define BIG_RESPONSE_DELAY        4900

/* Constants that aren't configurable in menuconfig */
//#define MQTT_URL "mqtt://iot.eclipse.org",
#define MQTT_URL "mqtt://192.168.137.100"
#define MQTT_PORT 1883



#define APP_WIFI_SSID CONFIG_WIFI_SSID
#define APP_WIFI_PASS CONFIG_WIFI_PASSWORD