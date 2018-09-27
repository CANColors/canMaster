#include <esp_err.h>
#include <esp_log.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>


#include "can.h"
#include "config.h"
#include "control.h"

extern SemaphoreHandle_t obd_tx_wait;

extern QueueHandle_t rxCanQueue;
extern QueueHandle_t txCanQueue;
extern QueueHandle_t controlEvents;

static const char *TAG="OBD";

void obd_wait_task(void *arg)
{
  
     while (1)
  {
     xSemaphoreTake(obd_tx_wait, portMAX_DELAY);
     
      vTaskDelay(pdMS_TO_TICKS(RESPONSE_DELAY));
      ESP_LOGI(TAG, "Send Response pending command" );
      
      can_msg_timestamped canM;
       canM.msg.identifier =  0x7E8;
       canM.msg.data_length_code = 8;
       canM.msg.flags = CAN_MSG_FLAG_NONE;
       canM.msg.data[0] = 0x78;
       canM.msg.data[1] = 0x00;                               
       canM.msg.data[2] = 0x00;
       canM.msg.data[3] = 0x00;
       canM.msg.data[4] = 0x00;
       canM.msg.data[5] = 0x00;
       canM.msg.data[6] = 0x00;
       canM.msg.data[7] = 0x00;
      
       xQueueSend(txCanQueue, &canM, portMAX_DELAY); 
      
      vTaskDelay(pdMS_TO_TICKS(BIG_RESPONSE_DELAY));
      ESP_LOGI(TAG, "BIG Delay finished" );
      ControlState cs2 = EV_BIG_TIMEOUT;    //return to start
      xQueueSend(controlEvents, &cs2, portMAX_DELAY);
  }

}
