
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "driver/can.h"

#include "can.h" 
#include "iso.h"

uint8_t blockSize = 0;
uint8_t STMin   = 0;

ISOStatus iso_send(uint8_t* data,  uint8_t len)
{

  can_message_t tx_msg;
  can_message_t rx_msg;
 
 ISOStatus isoStatus = ISOSTATUS_NONE; 
   esp_err_t res;
  uint8_t  bytesSent = 0;
   uint8_t have2Send = 0;
   
   
  tx_msg.identifier =  0x7E8;
  
  if (len < 7)
  {
    tx_msg.data_length_code = len+1;
    tx_msg.flags = CAN_MSG_FLAG_NONE;
    tx_msg.data[0]=len;
    for (int i=0; i<len;i++)
    {
      tx_msg.data[i+1]=data[i];
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
          if (rx_msg.data[0] == 0x30) 
          {
            blockSize = rx_msg.data[1];
            STMin =  rx_msg.data[2]; 
          }
          if (blockSize == 0)
          {
            have2Send = (len-6) % 7 + 1;
          } else have2Send = blockSize; 
          
          for (int j = 0; j<have2Send; j++)   //Block
          {
             tx_msg.data[0]=0x20+j;
             for (int k=0; k<7;k++)
             {
                tx_msg.data[k+1] = data[k+bytesSent];
             }
             
              if (STMin != 0) vTaskDelay(STMin / portTICK_PERIOD_MS);
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

 return isoStatus;

}
