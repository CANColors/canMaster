#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "config.h"
#include "control.h"
#include "requests.h"
#include "can.h"



SemaphoreHandle_t net_rx;
SemaphoreHandle_t net_tx;

extern QueueHandle_t rxCanQueue;
extern QueueHandle_t txCanQueue;
extern QueueHandle_t controlEvents;

void mqtt_receive(esp_mqtt_event_handle_t event);

static const char* TAG = "MQTT";

static esp_mqtt_client_handle_t client;


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_subscribe(client, MQTT_RESPONSE_TOPIC , 2);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

         /*
            msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
            ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);*/
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
      /*      msg_id = esp_mqtt_client_publish(client, "top", "dddd", 0, 0, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);*/
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            //TODO Тут мы принимает данные и их нужно обработать 
            mqtt_receive(event);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
    }
    return ESP_OK;
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void MqttInitM(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri  = MQTT_URL,
        .port = MQTT_PORT,
        .transport = MQTT_TRANSPORT_OVER_TCP,
        .event_handle = mqtt_event_handler,
        // .user_context = (void *)your_context
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/                                                                               
void mqtt_transmit_task(void *arg)
{
   UBaseType_t cntMsg; 
   uint16_t timeslot = 0;
   
       
  while (1)
  {
        
       xSemaphoreTake(net_tx, portMAX_DELAY);  
      cntMsg = uxQueueMessagesWaiting( rxCanQueue );
        
      if (cntMsg > 0) //Если есть что послать - посылаем
      {
      ESP_LOGI(TAG, "Messages to send:%d",  cntMsg );
       ControlEvents cs2 = EV_NET_START;    
       xQueueSend(controlEvents, &cs2, portMAX_DELAY);
       
        char* data = request_server_masterrequest();
         ESP_LOGI(TAG, "Message length:%d",  strlen(data));
        int msg_id = esp_mqtt_client_publish(client, MQTT_REQUEST_TOPIC, data, strlen(data), 2, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
       
        free (data);
        
       cs2 = EV_NET_END;    
       xQueueSend(controlEvents, &cs2, portMAX_DELAY);  
      }else 
          if (timeslot >= TIME_HEARTBEAT)    //  Иначе если пришло время для heartbeat - шлем его
          {
           // char* data = request_heartbeat();
           // post (HTTP_HEARTBEAT_PAGE, data);
          //  free (data);
            timeslot = 0;
          } 
     timeslot++;
        
       
     vTaskDelay( NET_TRANSMIT_TIMESLOT/ portTICK_PERIOD_MS);     
  
  }

}

/*****************************************************************************/
/*      По событию прихода данных на топик                                   */
/*   парсим их и отправляем дальше                                           */
/*****************************************************************************/         
void mqtt_receive(esp_mqtt_event_handle_t event)
{
    char* data; 
  //  if (!strcmp(event->topic, "request") )
     {
        data =   event->data;
        if (data != NULL)
        {
          CJSON_PUBLIC(cJSON *) jdata =  cJSON_Parse(data);
          if (!jdata )
          {
             ESP_LOGE(TAG, "JSON parse failed: %s", cJSON_GetErrorPtr());
          } else
          {
           // cJSON * trans = cJSON_GetObjectItem(jdata,"transaction");
           // int readed =  cJSON_GetObjectItem(trans,"readed")->valueint;
           // if (readed == 1)
            {
              request_can_send (jdata);
              cJSON_Delete (jdata);
             // goto new_data;
            } 
            // cJSON_Delete (jdata);
            
          }
          
          
        }
     }

} 

/*
void http_receive_task(void *arg)
{
   char* data; 
  while (1)
  {
     xSemaphoreTake(http_rx, portMAX_DELAY);
    ControlEvents cs2 = EV_HTTP_START;    
    xQueueSend(controlEvents, &cs2, portMAX_DELAY);
        char* req ;
  new_data:  req = request_response();
    data = get_data(HTTP_RESPONSE_DATA_PAGE, req);
 //   data = get_data(HTTP_RESPONSE_DATA_PAGE, req);
    // data = "{\"transaction\":	{\"responseId\":0,\"id\":	0,\"starttime\":	\"2683303892d\",\"waitresponce\":	1000,\"dataamount\":	3}}";
      if (data != NULL)
      {
        CJSON_PUBLIC(cJSON *) jdata =  cJSON_Parse(data);
        if (!jdata )
        {
           ESP_LOGE(HTTP_TAG, "JSON parse failed: %s", cJSON_GetErrorPtr());
        } else
        {
          cJSON * trans = cJSON_GetObjectItem(jdata,"transaction");
          int readed =  cJSON_GetObjectItem(trans,"readed")->valueint;
          if (readed == 1)
          {
            request_can_send (jdata);
            cJSON_Delete (jdata);
            goto new_data;
          } 
           cJSON_Delete (jdata);
          
        }
        
        
      }
    cs2 = EV_HTTP_END;    
    xQueueSend(controlEvents, &cs2, portMAX_DELAY);  
  }


}
    */