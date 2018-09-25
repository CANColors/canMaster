/* ========================================================================== */
/*                                                                            */
/*   Filename.c                                                               */
/*   (c) 2012 Author                                                          */
/*                                                                            */
/*   Description                                                              */
/*                                                                            */
/* ========================================================================== */
#include "esp_log.h"

#include "lwip/err.h"
//#include "D:\Work\CAN\SDK\esp-idf2\components\lwip\include\lwip\apps\sntp\sntp.h"
#include "lwip/apps/sntp.h"
#include "wifi.h"

static void obtain_time(void);
static void initialize_sntp(void);


static const char *TAG = "Timestamp";

void set_time(void)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
        // update 'now' variable with current time
        time(&now);
    }

    setenv("TZ", "CST-3", 1);
    tzset();
    
    char strftime_buf[100];
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time  is: %s", strftime_buf);
}

static void obtain_time(void)
{
   // ESP_ERROR_CHECK( nvs_flash_init() );
     wifi_wait_connected();
    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while(timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

   
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

int32_t get_timestamp(void) {
   struct timeval tv;
   gettimeofday(&tv, NULL);
   return (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));
}