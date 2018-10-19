
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <driver/can.h>
#include <esp_err.h>
#include <esp_log.h>

#include "can.h" 
#include "iso.h"

uint8_t blockSizeSend = 0;
uint8_t STMinSend   = 0;
uint8_t blockSizeReceive = 0;
uint8_t STMinReceive   = 0;

void sendFCF (can_message_t* msg, uint8_t bs,  uint8_t stMin);

extern QueueHandle_t txCanQueue;

static const char *TAG="ISO_TP";
/****************************************************************************/
/*                                                                          */
/****************************************************************************/
ISOStatus iso_send(uint8_t* data,  uint8_t len)
{

  can_message_t tx_msg;
  can_message_t rx_msg;
 
 ISOStatus isoStatus = ISOSTATUS_NONE; 
   esp_err_t res;
  uint8_t  bytesSent = 0;
   uint8_t have2Send = 0;
   
   
  tx_msg.identifier =  0x7E8;

//ESP_LOGI(TAG, "Iso send  = %d",  data[0]);  

  if (len <= 7)
  {
// ESP_LOGI(TAG, "Single Frame  = %d",  data[0]);  
    tx_msg.data_length_code = len+1;
    tx_msg.flags = CAN_MSG_FLAG_NONE;
    tx_msg.data[0]=len;
    for (int i=0; i<len;i++)
    {
      tx_msg.data[i+1]=data[i];
  //    ESP_LOGI(TAG, "Data to send [%d] = %d", i+1, data[i]);  
    }
    
    can_transmit(&tx_msg, portMAX_DELAY);  
   isoStatus = ISOSTATUS_OK;    
  }else  //MultyFrame
  {
       tx_msg.data_length_code = 8;
       tx_msg.flags = CAN_MSG_FLAG_NONE;
       tx_msg.data[0]=0x10;
       tx_msg.data[1]=len;
       
       for (int i=0; i<6;i++)
       {tx_msg.data[i+2]=data[i];}
       
       can_transmit(&tx_msg, portMAX_DELAY);
       bytesSent = 6;
  
     while (len - bytesSent > 0)
     {
               
       res = can_receive(&rx_msg, 1000); 
       if (res == ESP_OK)
        {
    //    ESP_LOGI(TAG, "Received FCF:%d", rx_msg.data[0]);  
          if (rx_msg.data[0] == 0x30) 
          {
            blockSizeSend = rx_msg.data[1];
            STMinSend =  rx_msg.data[2]; 
          }
          if (blockSizeSend == 0)
          {
            have2Send = (len-6) / 7 + (((len-6)%7)?1:0);
          } else have2Send = blockSizeSend; 
          
     //      ESP_LOGI(TAG, "len:%d, have2send:%d", len, have2Send);  
          for (int j = 0; j<have2Send; j++)   //Block
          {
             tx_msg.data[0]=0x20+j;
             for (int k=0; k<7;k++)
             {
                tx_msg.data[k+1] = data[k+bytesSent];
             }
                can_transmit(&tx_msg, portMAX_DELAY);
     //           ESP_LOGI(TAG, "transmitted:%d",  tx_msg.data[0]); 
                if (STMinSend != 0) vTaskDelay(STMinSend / portTICK_PERIOD_MS);
             bytesSent+=7;
          
          }
          isoStatus = ISOSTATUS_OK;    
        } else
        {
         isoStatus = ISOSTATUS_FAILED;    
         break;
        
        }
     }
  }
     // ESP_LOGI(TAG, "isoStatus = %d",  isoStatus);  
 return isoStatus;

}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/
ISOStatus iso_receiveMF(can_message_t* rx_msg, uint8_t* data,  uint8_t len)
{
  ISOStatus isoStatus = ISOSTATUS_NONE; 

  can_msg_timestamped msg_timestamped;
  
  uint8_t mfLen = 0;
  uint8_t mfCnt = 0;
  
    ESP_LOGI(TAG, "receive MF = %d", rx_msg->data[0]);
      
   if (rx_msg->data[0] == 0x10) //FirstFrame
   {
      mfLen = rx_msg->data[1];
      memcpy(data, &(rx_msg->data[2]), 6);
      mfCnt = 6; 
   
      sendFCF( rx_msg, blockSizeReceive, STMinReceive);
    
      while (mfCnt < mfLen)
      {
        BaseType_t resR =  xQueueReceive(txCanQueue, &msg_timestamped, portMAX_DELAY);
        if (resR == pdTRUE)
        {
            uint8_t n_pci =   msg_timestamped.msg.data[0] &&0xF0;
            
           if ( n_pci == 0x20) // Consequitive Frame
           {
              memcpy (data+mfCnt, &(msg_timestamped.msg.data[1]),7);
              mfCnt+=7;
           
           }
        } else
        {
          isoStatus = ISOSTATUS_FAILED;
          break; 
        }
      }
      isoStatus = ISOSTATUS_OK; 
    
   } 
  return isoStatus;
}

void sendFCF (can_message_t* msg, uint8_t bs,  uint8_t stMin)
{
      can_message_t tx_msg;
      tx_msg.identifier =  0x7E8;
      tx_msg.data_length_code = 8;
      tx_msg.flags = CAN_MSG_FLAG_NONE;
      tx_msg.data[0] = 0x30;
      tx_msg.data[1] = 0x00;                               
      tx_msg.data[2] = bs;
      tx_msg.data[3] = 0x00;
      tx_msg.data[4] = stMin;
      tx_msg.data[5] = 0x00;
      tx_msg.data[6] = 0x00;
      tx_msg.data[7] = 0x00;
       
     can_transmit(&tx_msg, portMAX_DELAY);
}