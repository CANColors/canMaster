/* ========================================================================== */
/*                                                                            */
/*   Filename.c                                                               */
/*   (c) 2012 Author                                                          */
/*                                                                            */
/*   Description                                                              */
/*                                                                            */
/* ========================================================================== */
#include <string.h>
#include <stdlib.h>
#include <esp_log.h>
#include <cjson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_http_client.h>

#include "can.h"
#include "config.h"
#include "requests.h"
#include "control.h"


#define HTTP_BUF_MAX 100000



extern QueueHandle_t rxCanQueue;
extern QueueHandle_t txCanQueue;
extern QueueHandle_t controlEvents;

extern SemaphoreHandle_t can_rx;
extern SemaphoreHandle_t can_tx;
extern SemaphoreHandle_t http_rx;
extern SemaphoreHandle_t http_tx;
extern SemaphoreHandle_t http_tx_wait;

static const char *HTTP_TAG = "HTTP";
char httpBuf[HTTP_BUF_MAX];
char* curPos= NULL;
int httpLen = 0;

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(HTTP_TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
        {
            ESP_LOGD(HTTP_TAG, "HTTP_EVENT_ON_CONNECTED");
            curPos = httpBuf;
            httpLen = 0;
            memset(httpBuf, 0, HTTP_BUF_MAX);
            break;
        }
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(HTTP_TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(HTTP_TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_MESSAGE_BEGIN:
        {
            curPos = httpBuf;
            httpLen = 0;
            memset(httpBuf, 0, HTTP_BUF_MAX);
            break;
        }    
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(HTTP_TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
             
            // esp_http_client client = evt->client;
            //if (client->method == HTTP_METHOD_POST)
            {
            if (esp_http_client_is_chunked_response(evt->client)) {
                // Write out data
                // printf("%.*s", evt->data_len, (char*)evt->data);
                memcpy(curPos,evt->data, evt->data_len ) ;;
                curPos += evt->data_len;
                httpLen += evt->data_len;
             //   ESP_LOGD(HTTP_TAG, "HTTP_EVENT_ON_DATA, len=%d, data=%s", evt->data_len, (char*)evt->data);
            }
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(HTTP_TAG, "HTTP_EVENT_ON_FINISH");
            curPos = httpBuf;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(HTTP_TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}



void post( const char* page, const char *post_data)
{
       esp_http_client_config_t config = {
        .url = WEB_URL,
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err;// = esp_http_client_perform(client);

    // POST
   // const char *post_data1 = "&field1=value1&field2=value2";
   char url [sizeof(WEB_URL) + sizeof(page)+10];
   memset (url,0, sizeof(url));
   strcpy (url, WEB_URL);
   strcat (url, page);
   ESP_LOGI(HTTP_TAG, "URL:%s", url);
    esp_http_client_set_url(client, url);
   
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client, "Content-Type", "application/json");
    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(HTTP_TAG, "HTTP POST Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(HTTP_TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
  esp_http_client_cleanup(client);  
}

char* get_data( const char* page, const char *post_data)
{
      esp_http_client_config_t config = {
        .url = WEB_URL,
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err ;//= esp_http_client_perform(client);

    // POST
   //const char *post_data1 = "?message=***";
   char url [sizeof(WEB_URL) + sizeof(page)+10];
   memset (url,0, sizeof(url));
   strcpy (url, WEB_URL);
   strcat (url, page);
   ESP_LOGI(HTTP_TAG, "URL:%s", url);
    esp_http_client_set_url(client, url);
   
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client, "Content-Type", "application/json");
    err = esp_http_client_perform(client);
    
    char* pdata;
    esp_http_client_get_post_field(client, &pdata);
    ESP_LOGI(HTTP_TAG, "REQ:%s", pdata);
    
    if (err == ESP_OK) 
    {
      int len = esp_http_client_get_content_length(client);
    
        ESP_LOGI(HTTP_TAG, "HTTP POST Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                len);
        
   
                
    }
    /* else {
        ESP_LOGE(HTTP_TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
        data = NULL;
    } */
  
    
  esp_http_client_cleanup(client); 
  return httpBuf; 
}


void http_transmit_task(void *arg)
{
   UBaseType_t cntMsg; 
   uint16_t timeslot = 0;
    
  while (1)
  {
        
       xSemaphoreTake(http_tx, portMAX_DELAY);  
      cntMsg = uxQueueMessagesWaiting( rxCanQueue );
        
      if (cntMsg > 0) //Если есть что послать - посылаем
      {
      ESP_LOGI(HTTP_TAG, "Messages to send:%d",  cntMsg );
       ControlEvents cs2 = EV_HTTP_START;    
       xQueueSend(controlEvents, &cs2, portMAX_DELAY);
       
        char* data = request_server_masterrequest();
        post (HTTP_REQUEST_DATA_PAGE, data);
        free (data);
        
       cs2 = EV_HTTP_END;    
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
        
       
     vTaskDelay( HTTP_TRANSMIT_TIMESLOT/ portTICK_PERIOD_MS);     
  
  }

}

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


void http_wait_task(void *arg)
{
  
     while (1)
  {
     xSemaphoreTake(http_tx_wait, portMAX_DELAY);
     
      vTaskDelay(pdMS_TO_TICKS(RESPONSE_DELAY));
      ESP_LOGI(HTTP_TAG, "Send Response pending command" );
      
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
      ESP_LOGI(HTTP_TAG, "BIG Delay finished" );
      ControlState cs2 = EV_BIG_TIMEOUT;    //return to start
      xQueueSend(controlEvents, &cs2, portMAX_DELAY);
  }

}
