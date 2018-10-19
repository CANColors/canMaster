#include <esp_err.h>
#include <esp_log.h>

#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>



#include "can.h"
#include "config.h"
#include "control.h"
#include "obd.h"
#include "iso.h"

#define SERVICE01 1
#define SERVICE09 1

extern SemaphoreHandle_t obd_tx_wait;

extern QueueHandle_t rxCanQueue;
extern QueueHandle_t txCanQueue;
extern QueueHandle_t controlEvents;

static const char *TAG="OBD";

OBDResponseStatus Service01(can_message_t *rx_msg );
OBDResponseStatus Service09(can_message_t *rx_msg );
OBDResponseStatus Service01Response(can_message_t *rx_msg );
OBDResponseStatus Service09Response(can_message_t *rx_msg );

OBDPid  S01_P00;
OBDPid  S01_P20;
OBDPid  S01_P40;
OBDPid  S01_P60;
OBDPid  S01_P0C;

OBDPid  S09_P02;


/****************************************************************************/
/*                                                                          */
/****************************************************************************/
void OBDInit(void)
{
    S01_P00.service = 01;
    S01_P00.pid = 0x00;
    S01_P00.resLen = 7;
    S01_P00.response = calloc (7, sizeof(uint8_t));
    S01_P00.response[0] = 0x41;
    S01_P00.response[1] = 0x00;
    S01_P00.response[2] = 0xFF;
    S01_P00.response[3] = 0xFF;
    S01_P00.response[4] = 0xFF;
    S01_P00.response[5] = 0xFF;
    S01_P00.response[6] = 0x00;
    S01_P00.requestState = OBD_REQUEST_NONE;
    
    S01_P20.service = 01;
    S01_P20.pid = 0x20;
    S01_P20.resLen = 7;
    S01_P20.response = calloc (7, sizeof(uint8_t));
    S01_P20.response[0] = 0x41;
    S01_P20.response[1] = 0x20;
    S01_P20.response[2] = 0xFF;
    S01_P20.response[3] = 0xFF;
    S01_P20.response[4] = 0xFF;
    S01_P20.response[5] = 0xFF; 
    S01_P20.response[6] = 0x00;
    S01_P20.requestState = OBD_REQUEST_NONE;
    
    S01_P40.service = 01;
    S01_P40.pid = 0x40;
    S01_P40.resLen = 7;
    S01_P40.response = calloc (7, sizeof(uint8_t));
    S01_P40.response[0]= 0x41; 
    S01_P40.response[1]= 0x40;
    S01_P40.response[2]= 0xFF;
    S01_P40.response[3]= 0xFF;
    S01_P40.response[4]= 0xFF;
    S01_P40.response[5]= 0xFF;
    S01_P40.response[6]= 0x00;
    S01_P40.requestState = OBD_REQUEST_NONE;
      
    S01_P60.service = 01;
    S01_P60.pid = 0x60;
    S01_P60.resLen = 7;
    S01_P60.response = calloc (7, sizeof(uint8_t));
    S01_P60.response[0]= 0x41; 
    S01_P60.response[1]= 0x40;
    S01_P60.response[2]= 0xFF;
    S01_P60.response[3]= 0xFF;
    S01_P60.response[4]= 0xFF;
    S01_P60.response[5]= 0xFF;
    S01_P60.response[6]= 0x00;
    S01_P60.requestState = OBD_REQUEST_NONE;
    
    S01_P0C.service = 01;
    S01_P0C.pid = 0x0C;
    S01_P0C.resLen = 7;
    S01_P0C.response = calloc (7, sizeof(uint8_t));
    S01_P0C.response[0]= 0x41;
    S01_P0C.response[1]= 0x0C;
    S01_P0C.response[2]= 0x0F;
    S01_P0C.response[3]= 0xF0;
    S01_P0C.response[4]= 0x00;
    S01_P0C.response[5]= 0x00;
    S01_P0C.response[6]= 0x00; 
    S01_P0C.requestState = OBD_REQUEST_NONE;
    
    S09_P02.service = 01;
    S09_P02.pid = 0x0C;
    S09_P02.resLen = 34;
    S09_P02.response = calloc (34, sizeof(uint8_t));
    S09_P02.response[0]= 0x49;
    S09_P02.response[1]= 0x02;
    S09_P02.response[2]= 0x01;
    S09_P02.response[3]= 0x00;
    S09_P02.response[4]= 0x00;
    S09_P02.response[5]= 0x00;
    S09_P02.response[6]= 0x49;
    S09_P02.response[7]= 0x02;
    S09_P02.response[8]= 0x02;
    S09_P02.response[9]= 0x47;
    S09_P02.response[10]= 0x31;
    S09_P02.response[11]= 0x4A;
    S09_P02.response[12]= 0x43;
    S09_P02.response[13]= 0x49;
    S09_P02.response[14]= 0x02;
    S09_P02.response[15]= 0x03;
    S09_P02.response[16]= 0x35;
    S09_P02.response[17]= 0x34;
    S09_P02.response[18]= 0x34;
    S09_P02.response[19]= 0x34;
    S09_P02.response[20]= 0x49;
    S09_P02.response[21]= 0x02;
    S09_P02.response[22]= 0x04;
    S09_P02.response[23]= 0x52;
    S09_P02.response[24]= 0x37;
    S09_P02.response[25]= 0x32;
    S09_P02.response[26]= 0x35;
    S09_P02.response[27]= 0x49;
    S09_P02.response[28]= 0x02;
    S09_P02.response[29]= 0x05;
    S09_P02.response[30]= 0x32;
    S09_P02.response[31]= 0x33;
    S09_P02.response[32]= 0x36;
    S09_P02.response[33]= 0x37;
    S09_P02.requestState = OBD_REQUEST_NONE;
    
}


/****************************************************************************/
/*                                                                          */
/****************************************************************************/
OBDResponseStatus OBDResponse (can_message_t *rx_msg)
 {
      
      OBDResponseStatus status = OBD_REQUEST_NONE; 
       // if (isAddressGood(rx_msg.identifier))    
        //if (rx_msg.identifier == 0x7DF || rx_msg.identifier ==0x07E0 ) {
               
          {
            ESP_LOGI(TAG, "Received Service %d", rx_msg->data[1] );
           switch (rx_msg->data[1])
           {
            case 0x01: {status = Service01(rx_msg); break;}
            case 0x09: {status = Service09(rx_msg); break;}
            default:{status = OBD_REQUEST_NONE; break;}
           }
       }
 return status;
 }
 
/****************************************************************************/
/*                                                                          */
/****************************************************************************/
 OBDResponseStatus OBDSend (can_msg_timestamped *msg_timestamped)
{
   OBDResponseStatus status = OBD_REQUEST_NONE; 
       // if (isAddressGood(rx_msg.identifier))    
        //if (rx_msg.identifier == 0x7DF || rx_msg.identifier ==0x07E0 ) {
               
          {
            ESP_LOGI(TAG, "Received Service [1]=%d, [2] = %d", msg_timestamped->msg.data[1], msg_timestamped->msg.data[2] );
           switch (msg_timestamped->msg.data[1])
           {
            case 0x41: {status = Service01Response(&msg_timestamped->msg); break;}
            case 0x49: {status = Service09Response(&msg_timestamped->msg); break;}
            default:{status = OBD_REQUEST_NONE; break;}
           }
       }
 return status; 
  
  
}
 
 
/****************************************************************************/
/*                                                                          */
/****************************************************************************/ 
OBDResponseStatus Service01Response(can_message_t *rx_msg )
{
OBDResponseStatus res = OBD_RESPONSE_NONE;


   // ISOStatus isoStatus;
    
    
   //  ESP_LOGI(TAG, "Service = %d  PID = %d ", rx_msg->data[1], rx_msg->data[2] );
        
    
    switch (rx_msg->data[2])
    {
      case 0x00:
      {
        S01_P00.requestState = OBD_REQUEST_NONE;
        memset (S01_P00.response, 0, S01_P00.resLen);
        
        S01_P00.resLen =   rx_msg->data_length_code-1;
        memcpy(S01_P00.response,(rx_msg->data)+1, S01_P00.resLen);
              
        break;
      }
    
      case 0x20:
      {
        S01_P20.requestState = OBD_REQUEST_NONE;
        memset (S01_P20.response, 0, S01_P20.resLen);
        
        if (rx_msg->data_length_code-1 >  S01_P20.resLen)
        {  realloc(S01_P20.response, rx_msg->data_length_code-1 );
          S01_P20.resLen =   rx_msg->data_length_code-1;
          }
        
        memcpy(S01_P20.response, (rx_msg->data)+1, S01_P20.resLen);
              
      }
     
     case 0x40:
      {
        S01_P40.requestState = OBD_REQUEST_NONE;
        memset (S01_P40.response, 0, S01_P40.resLen);
        
        if (rx_msg->data_length_code-1 >  S01_P40.resLen)
        {  realloc(S01_P40.response, rx_msg->data_length_code-1 );
          S01_P40.resLen =   rx_msg->data_length_code-1;
          }
        
        memcpy(S01_P40.response, (rx_msg->data)+1, S01_P40.resLen);
        break;
      }
    
    case 0x60:
      {
       S01_P60.requestState = OBD_REQUEST_NONE;
        memset (S01_P60.response, 0, S01_P60.resLen);
        
        if (rx_msg->data_length_code-1 >  S01_P60.resLen)
        {  realloc(S01_P60.response, rx_msg->data_length_code-1 );
          S01_P60.resLen =   rx_msg->data_length_code-1;
          }
        
        memcpy(S01_P60.response, (rx_msg->data)+1, S01_P60.resLen);
        break;
      }
    
    case 0x0C:
      {
     //   ESP_LOGI(TAG, "S01_P0C.resLen %d, rx_msg->data_length_code %d ", S01_P0C.resLen, rx_msg->data_length_code );
      S01_P0C.requestState = OBD_REQUEST_NONE;
        memset (S01_P0C.response, 0, S01_P0C.resLen);
        
        if (rx_msg->data_length_code-1 >  S01_P0C.resLen) 
        { 
            realloc(S01_P0C.response, rx_msg->data_length_code-1 );
            S01_P0C.resLen =   rx_msg->data_length_code-1;
        }
        
        memcpy(S01_P0C.response, (rx_msg->data)+1, S01_P0C.resLen);
          
          ESP_LOGI(TAG, "RPM refreshed ");
        break;
      }
      
         
     default:
     {
      res = OBD_RESPONSE_NONE;
      break;
     }

    }
    


  return res;    
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/
 
OBDResponseStatus Service01(can_message_t *rx_msg )
{
OBDResponseStatus res = OBD_RESPONSE_NONE;
#ifdef SERVICE01

      ISOStatus isoStatus = ISOSTATUS_NONE;
    
    
     ESP_LOGI(TAG, "Service = %d  PID = %d ", rx_msg->data[1], rx_msg->data[2] );
        
    
    switch (rx_msg->data[2])
    {
      case 0x00:
      {
        if (S01_P00.requestState == OBD_REQUEST_NONE)
        {
           res = OBD_RESPONSE_NEED_TO_SEND;
        }
        else  res = OBD_RESPONSE_NONE;
                     
        isoStatus = iso_send(S01_P00.response, S01_P00.resLen);
        if (isoStatus != ISOSTATUS_OK) res = OBS_RESPONSE_ERROR_NETWORK;
        
        
        break;
      }
    
      case 0x20:
      {
        if (S01_P20.requestState == OBD_REQUEST_NONE)
        {
           res = OBD_RESPONSE_NEED_TO_SEND;
        }
        else  res = OBD_RESPONSE_NONE;
                     
       isoStatus =  iso_send(S01_P20.response, S01_P20.resLen);
        if (isoStatus != ISOSTATUS_OK) res = OBS_RESPONSE_ERROR_NETWORK;
        break;
      }
     
     case 0x40:
      {
        if (S01_P40.requestState == OBD_REQUEST_NONE)
        {
           res = OBD_RESPONSE_NEED_TO_SEND;
        }
        else  res = OBD_RESPONSE_NONE;
       isoStatus =  iso_send(S01_P40.response, S01_P40.resLen);
        if (isoStatus != ISOSTATUS_OK) res = OBS_RESPONSE_ERROR_NETWORK;
        break;
      }
    
    case 0x60:
      {
        if (S01_P60.requestState == OBD_REQUEST_NONE)
        {
           res = OBD_RESPONSE_NEED_TO_SEND;
        }
        else  res = OBD_RESPONSE_NONE;
                     
       isoStatus =  iso_send(S01_P60.response, S01_P60.resLen);
        if (isoStatus != ISOSTATUS_OK) res = OBS_RESPONSE_ERROR_NETWORK;
        break;
      }
    
    case 0x0C:
      {
        if (S01_P0C.requestState == OBD_REQUEST_NONE)
        {
           res = OBD_RESPONSE_NEED_TO_SEND;
        }
        else  res = OBD_RESPONSE_NONE;
                     
      isoStatus =  iso_send(S01_P0C.response, S01_P0C.resLen);
              
       if (isoStatus != ISOSTATUS_OK) res = OBS_RESPONSE_ERROR_NETWORK;
        break;
      }
      
         
     default:
     {
      res = OBD_RESPONSE_NONE;
      break;
     }

    }
    
#endif 

  return res;    
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/
OBDResponseStatus  Service09  (can_message_t *rx_msg )
{
OBDResponseStatus res = OBD_RESPONSE_NONE;
#ifdef SERVICE09
       ISOStatus isoStatus = ISOSTATUS_NONE;
  
   switch (rx_msg->data[2])
   {
    case 0x02:          //VIN Request
    {
    
     if (S09_P02.requestState == OBD_REQUEST_NONE)
        {
           res = OBD_RESPONSE_NEED_TO_SEND;
        }
        else  res = OBD_RESPONSE_NONE;
                     
       isoStatus = iso_send(S09_P02.response, S09_P02.resLen);
        if (isoStatus != ISOSTATUS_OK) res = OBS_RESPONSE_ERROR_NETWORK;
        ESP_LOGI(TAG, "VIN transmitted ");
        break;
     }
     
      default:
     {
      res = OBD_RESPONSE_NONE;
      break;
     }
      
    }
   
   
   
#endif
 return res;    
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/   
OBDResponseStatus  Service09Response  (can_message_t *rx_msg )
{
  OBDResponseStatus res = OBD_RESPONSE_NONE;
  ISOStatus isoStatus = ISOSTATUS_NONE;
     
   switch (rx_msg->data[2])
   {
    case 0x02:          //VIN Request
    {
    
     if (S09_P02.requestState == OBD_REQUEST_NONE)
        {
           res = OBD_RESPONSE_NEED_TO_SEND;
        }
        else  res = OBD_RESPONSE_NONE;
                     
        isoStatus = iso_receiveMF( rx_msg, S09_P02.response, S09_P02.resLen);
        if (isoStatus != ISOSTATUS_OK) res = OBS_RESPONSE_ERROR_NETWORK;
        ESP_LOGI(TAG, "VIN received ");
        break;
     }
     
      default:
     {
      res = OBD_RESPONSE_NONE;
      break;
     }
      
    }
   
   
   

 return res;    
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

/* void obd_wait_task(void *arg)
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
       canM.msg.data[0] = 0x01;
       canM.msg.data[1] = 0x78;                               
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
  */
  
  
void obd_wait_task(void *arg)
{
     xSemaphoreTake(obd_tx_wait, portMAX_DELAY);
     while (1)
  {
       S01_P00.requestState = OBD_REQUEST_NONE;
       S01_P20.requestState = OBD_REQUEST_NONE;
       S01_P40.requestState = OBD_REQUEST_NONE;
       S01_P60.requestState = OBD_REQUEST_NONE;
       S01_P0C.requestState = OBD_REQUEST_NONE;
       
       S09_P02.requestState = OBD_REQUEST_NONE;
        watchdog();
        vTaskDelay(pdMS_TO_TICKS(1000));
  }

}