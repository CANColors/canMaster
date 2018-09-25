/* ========================================================================== */
/*                                                                            */
/*   Filename.c                                                               */
/*   (c) 2012 Author                                                          */
/*                                                                            */
/*   Description                                                              */
/*                                                                            */
/* ========================================================================== */

#define MASTER_ROLE 1
#ifdef MASTER_ROLE

  #define HTTP_REQUEST_DATA_PAGE      "masterrequest"
  #define HTTP_HEARTBEAT_PAGE         "heartbeat"
  #define HTTP_RESPONSE_DATA_PAGE     "masterresponce"
 // #define HTTP_RESPONSE_DATA_PAGE     "echo"
  
#else
  #define HTTP_REQUEST_DATA_PAGE      "clientrequest"
  #define HTTP_HEARTBEAT_PAGE         "heartbeat"
  #define HTTP_RESPONSE_DATA_PAGE     "masterresponse"

#endif


#define HTTP_TRANSMIT_TIMESLOT  100
#define TIME_HEARTBEAT        5000
#define RESPONSE_DELAY        1
#define BIG_RESPONSE_DELAY        4900

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "748775.ikabis.web.hosting-test.net/masterrequest"
#define WEB_URL "http://748775.ikabis.web.hosting-test.net/raven/"
//#define WEB_SERVER 192.168.137.112
//#define WEB_URL "http://192.168.137.112/"
#define WEB_PORT 80



#define APP_WIFI_SSID CONFIG_WIFI_SSID
#define APP_WIFI_PASS CONFIG_WIFI_PASSWORD