
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_err.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>







#include "can.h"
#include "wifi.h"
#include "mqtt.h"
#include "timestamp.h"
#include "control.h"
#include "obd.h"


#define NET_RX_TASK_PRIO            8
#define NET_TX_TASK_PRIO            8
#define CAN_TX_TASK_PRIO          9
#define CAN_RX_TASK_PRIO          9
#define OBD_TX_TASK_PRIO          9

#define CONTROL_TASK_PRIO          9

#define CAN_RX_BUFFER   10
#define CAN_TX_BUFFER   10



static const char *TAG="MASTER";

extern QueueHandle_t rxCanQueue;
extern QueueHandle_t txCanQueue;
extern QueueHandle_t controlEvents;

SemaphoreHandle_t can_rx;
SemaphoreHandle_t can_tx;
SemaphoreHandle_t net_rx;
SemaphoreHandle_t net_tx;
SemaphoreHandle_t obd_tx_wait;

uint16_t cntTransaction = 0;

 
extern const int CONNECTED_BIT;


  
void app_main()
{
   
    ESP_ERROR_CHECK(nvs_flash_init());
    
     can_rx = xSemaphoreCreateBinary();
     can_tx = xSemaphoreCreateBinary();
     
     net_rx = xSemaphoreCreateBinary();
     net_tx = xSemaphoreCreateBinary();
     obd_tx_wait = xSemaphoreCreateBinary();
     
    rxCanQueue = xQueueCreate( CAN_RX_BUFFER, sizeof( can_msg_timestamped) );
    txCanQueue = xQueueCreate( CAN_TX_BUFFER, sizeof( can_msg_timestamped) );
    
   
     
    wifi_initialise(NULL);
    wifi_wait_connected();
    
    set_time();
    MqttInitM();
    canInit();
    OBDInit();
    
   //  xTaskCreatePinnedToCore(net_receive_task,  "NET_rx", 4096, NULL, NET_RX_TASK_PRIO, NULL, tskNO_AFFINITY);
     xTaskCreatePinnedToCore(mqtt_transmit_task, "NET_tx", 4096, NULL, NET_TX_TASK_PRIO, NULL, tskNO_AFFINITY);
     
   
     xTaskCreatePinnedToCore(obd_wait_task,     "OBD",    4096, NULL, OBD_TX_TASK_PRIO, NULL, tskNO_AFFINITY);
     xTaskCreatePinnedToCore(can_transmit_task, "CAN_tx", 4096, NULL, CAN_TX_TASK_PRIO, NULL, tskNO_AFFINITY);
     xTaskCreatePinnedToCore(can_receive_task,  "CAN_rx", 4096, NULL, CAN_RX_TASK_PRIO, NULL, tskNO_AFFINITY);
   
     xTaskCreatePinnedToCore(control_task,  "control", 4096, NULL,  CONTROL_TASK_PRIO , NULL, tskNO_AFFINITY);
   
   ESP_LOGI(TAG, "Ready to communications" ); 
   
     
  // xSemaphoreGive(http_rx);
  
   
}
