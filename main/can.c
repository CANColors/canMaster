/* ========================================================================== */
/*                                                                            */
/*   Filename.c                                                               */
/*   (c) 2012 Author                                                          */
/*                                                                            */
/*   Description                                                              */
/*                                                                            */
/* ========================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/can.h"

#include "can.h"  
#include "timestamp.h"
#include "control.h"

/* --------------------- Definitions and static variables ------------------ */
//canSniffer Configuration
#define RX_TASK_PRIO                    9
#define TX_GPIO_NUM                     4
#define RX_GPIO_NUM                     5
#define CAN_EN_GPIO           22
#define RX_BUFFER_MAX                   20
#define CAN_TAG                     "CAN"

#define OBD_VIN 1
//#define EMU_VIN 2
#define SERVICE01 1
//#define SERVICE09 1


extern SemaphoreHandle_t can_rx;
extern SemaphoreHandle_t can_tx;
extern SemaphoreHandle_t http_rx;
extern SemaphoreHandle_t http_tx;
extern SemaphoreHandle_t http_tx_wait;

QueueHandle_t rxCanQueue;
QueueHandle_t txCanQueue;
extern QueueHandle_t controlEvents;

void testPrintQueue(void);
uint8_t FakeResponse (can_message_t *rx_msg);
void Service01(can_message_t *rx_msg );
void Service09(can_message_t *rx_msg );

void setFakeRPM(void);
void sendRPM(can_message_t *tx );
void storeRPM(can_message_t *rx );



static const can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();
static const can_timing_config_t t_config = CAN_TIMING_CONFIG_500KBITS();
//Set TX queue length to 0 due to listen only mode
static const can_general_config_t g_config = {.mode = CAN_MODE_NORMAL,
                                              .tx_io = TX_GPIO_NUM, .rx_io = RX_GPIO_NUM,
                                              .clkout_io = CAN_IO_UNUSED, .bus_off_io = CAN_IO_UNUSED,
                                              .tx_queue_len = 5, .rx_queue_len = 5,
                                              .alerts_enabled = CAN_ALERT_NONE,
                                              .clkout_divider = 0};

uint8_t cntRxCan = 0;

can_message_t bufRPM;
uint8_t fakeRPMflag = 1;   
uint8_t waitRPMflag = 0;
uint8_t sendRPMflag = 0;


/* --------------------------- Tasks and Functions -------------------------- */

void can_receive_task(void *arg)
{
    xSemaphoreTake(can_rx, portMAX_DELAY);
   
   // 
   // testCanDataGenerate();
     ESP_LOGI(CAN_TAG, "Wait for CAN");    
    while (1) {
            
        can_message_t rx_msg;
        esp_err_t res = can_receive(&rx_msg, portMAX_DELAY);
        if (res == ESP_OK)
        {  
        ESP_LOGI(CAN_TAG, "REceived");
        can_msg_timestamped msg_timestamped;
        msg_timestamped.id = cntRxCan++;
        msg_timestamped.timestamp = get_timestamp();
        msg_timestamped.msg.identifier = rx_msg.identifier;
        msg_timestamped.msg.data_length_code = rx_msg.data_length_code;
        for (int i=0; i<rx_msg.data_length_code; i++)
        {
          msg_timestamped.msg.data[i] = rx_msg.data[i];
        }
        
       FakeResponse (&rx_msg); 
              //printCanMessage(&rx_msg,iterations);
      
       
       if (rx_msg.data[1] == 0x09 && rx_msg.data[2] == 0x02) goto NextStep; 
       
      if (waitRPMflag == 1 && rx_msg.data[3] == 0x0C)
       {
          waitRPMflag++;
          
          ControlEvents cs2 = EV_CAN_RECEIVED;    
          xQueueSend(controlEvents, &cs2, portMAX_DELAY);
          xQueueSend(rxCanQueue, &msg_timestamped, portMAX_DELAY);
       }
       
       if (waitRPMflag == 0 )
      {
       ControlEvents cs2 = EV_CAN_RECEIVED;    
       xQueueSend(controlEvents, &cs2, portMAX_DELAY);
       
        xQueueSend(rxCanQueue, &msg_timestamped, portMAX_DELAY);
      
      }
      
      NextStep:sendRPMflag = 3;
      
      }
    }

    
    vTaskDelete(NULL);
}

void can_transmit_task(void *arg)
{
    can_msg_timestamped msg_timestamped;
    
    
   // xSemaphoreTake(tx_sem, portMAX_DELAY);
    xSemaphoreTake(can_tx, portMAX_DELAY); 
    while (1)
    {
      
       BaseType_t res =  xQueueReceive(txCanQueue, &msg_timestamped, portMAX_DELAY);
        if (res == pdTRUE)
        { //Тут ещё нужно прикрутить управаление временем отправки в соответствии с таймстампом.
          // Может кагда-то понадобится.
          
          if (msg_timestamped.msg.data[1] == 0x41 && msg_timestamped.msg.data[2] == 0x0C )
          {
            storeRPM(&msg_timestamped.msg);
            waitRPMflag=0;    
          } else
          {
          ESP_LOGI(CAN_TAG, "Send to CAN");   
          msg_timestamped.msg.flags = CAN_MSG_FLAG_NONE;
          can_transmit(&msg_timestamped.msg, portMAX_DELAY);
          }   
             ControlEvents cs2 = EV_CAN_TRANSMITTED;    
             xQueueSend(controlEvents, &cs2, portMAX_DELAY);
          
        }
    
    }
    
 }  

//---------------------------------------------------------------//
void canInit(void)
{
//Enable CAN bus output
    gpio_pad_select_gpio(CAN_EN_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(CAN_EN_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(CAN_EN_GPIO, 0);
    
  
  
           
    //Install and start CAN driver
    ESP_ERROR_CHECK(can_driver_install(&g_config, &t_config, &f_config));
    ESP_LOGI(CAN_TAG, "Driver installed");
    ESP_ERROR_CHECK(can_start());
    ESP_LOGI(CAN_TAG, "Driver started");

    setFakeRPM();
  //  xSemaphoreGive(rx_sem);                     //Start RX task
}

//---------------------------------------------------------------//
/*void testCanDataGenerate(void)
{
  can_msg_timestamped msg_timestamped;
  
    for (int i=0;i<10;i++)
    {
        msg_timestamped.id = i;
        msg_timestamped.timestamp = get_timestamp();
        msg_timestamped.msg.identifier = 0x7DE;
        msg_timestamped.msg.data_length_code = 8;
        for (int j=0;j<8;j++)
        {
          msg_timestamped.msg.data[j] = i;
        }
        xQueueSend(rxCanQueue, &msg_timestamped, portMAX_DELAY);
       // vTaskDelay(50 / portTICK_PERIOD_MS);
    }
   
//  testPrintQueue();  

}
  */
/*
void testPrintQueue(void)
{
    UBaseType_t cntMsg = uxQueueMessagesWaiting( rxCanQueue );
    ESP_LOGI("PrintQueue", "cntMsg =>  %d", cntMsg );

    can_msg_timestamped msg;
    char buf [50];
     BaseType_t res =  xQueueReceive(rxCanQueue, &msg, 0);
     ESP_LOGI("PrintQueue", "MsgId =>  %d", msg.id );
     sprintf(buf,"%ud", msg.timestamp);
     ESP_LOGI("PrintQueue", "MsgTimestamp =>  %d", msg.timestamp );
     ESP_LOGI("PrintQueue", "MsgAddress =>  %d", msg.msg.identifier );
     ESP_LOGI("PrintQueue", "MsgData0 =>  %d", msg.msg.data[0] );
     
   
}
     
  */     
 
uint8_t FakeResponse (can_message_t *rx_msg)
 {
        uint8_t flag = 0;
       // if (isAddressGood(rx_msg.identifier))    
        //if (rx_msg.identifier == 0x7DF || rx_msg.identifier ==0x07E0 ) {
               
          {
            ESP_LOGI(CAN_TAG, "Received Service %d", rx_msg->data[1] );
           switch (rx_msg->data[1])
           {
            case 0x01: { Service01(rx_msg); flag = 1; break;}
            case 0x09: { Service09(rx_msg); flag = 1; break;}
            default:break;
           }
        }
 return flag;
 }
 
 

void Service01(can_message_t *rx_msg )
{
#ifdef SERVICE01

    can_message_t tx;
    can_message_t *tx_msg = &tx;
    
     ESP_LOGI(CAN_TAG, "PID = %d ",rx_msg->data[2] );
     tx_msg->identifier =  0x7E8;
     tx_msg->data_length_code = 8;
     tx_msg->flags = CAN_MSG_FLAG_NONE;
       
    switch (rx_msg->data[2])
    {
      case 0x00:
      {
         tx_msg->data[0] = 0x06;
         tx_msg->data[1] = 0x41;
         tx_msg->data[2] = 0x00;
         tx_msg->data[3] = 0xFF;
         tx_msg->data[4] = 0xFF;
         tx_msg->data[5] = 0xFF;
         tx_msg->data[6] = 0x00;
         tx_msg->data[7] = 0x00;
        break;
      }
    
     case 0x20:
      {
         tx_msg->data[0] = 0x06;
         tx_msg->data[1] = 0x41;
         tx_msg->data[2] = 0x20;
         tx_msg->data[3] = 0xFF;
         tx_msg->data[4] = 0xFF;
         tx_msg->data[5] = 0xFF;
         tx_msg->data[6] = 0xFF;
         tx_msg->data[7] = 0x00;
        break;
      }
    
     case 0x40:
      {
         tx_msg->data[0] = 0x06;
         tx_msg->data[1] = 0x41;
         tx_msg->data[2] = 0x40;
         tx_msg->data[3] = 0xFF;
         tx_msg->data[4] = 0xFF;
         tx_msg->data[5] = 0xFF;
         tx_msg->data[6] = 0xFF;
         tx_msg->data[7] = 0x00;
        break;
      }
    
     case 0x60:
      {
         tx_msg->data[0] = 0x06;
         tx_msg->data[1] = 0x41;
         tx_msg->data[2] = 0x60;
         tx_msg->data[3] = 0xFF;
         tx_msg->data[4] = 0xFF;
         tx_msg->data[5] = 0xFF;
         tx_msg->data[6] = 0xFF;
         tx_msg->data[7] = 0x00;
        break;
      }
    
       case 0x0C:
       {
        ESP_LOGI(CAN_TAG, " fake RPM =>  %d", fakeRPMflag );
       
       if (waitRPMflag== 0)
       { 
          waitRPMflag = 1;
          sendRPM(tx_msg);
         }
        break;
       }
     
     default:break;
    }

     ESP_LOGI(CAN_TAG, "PID = %d transmitted",rx_msg->data[2] );
     can_transmit(tx_msg, portMAX_DELAY);
#endif     
}

void Service09  (can_message_t *rx_msg )
{
#ifdef SERVICE09

  if (rx_msg->data[0] == 0x30) //FCF
  {
    
  
  } 
   
    can_message_t tx;
    can_message_t *tx_msg = &tx;
    
     ESP_LOGI(CAN_TAG, "PID = %d ",rx_msg->data[2] );
     tx_msg->identifier =  0x7E8;
     tx_msg->data_length_code = 8;
     tx_msg->flags = CAN_MSG_FLAG_NONE;
       
   switch (rx_msg->data[2])
   {
    case 0x02:          //VIN Request
    {
    
  #ifdef EMU_VIN
  ESP_LOGI(CAN_TAG, "EMU VIN  ");
       tx_msg->data[0] = 0x10;
       tx_msg->data[1] = 0x14;
       tx_msg->data[2] = 0x40;
       tx_msg->data[3] = 0x02;
       tx_msg->data[4] = 0x01;
       tx_msg->data[5] = 0x57;
       tx_msg->data[6] = 0x50;
       tx_msg->data[7] = 0x30;
      can_transmit(tx_msg, portMAX_DELAY);
      vTaskDelay(10 / portTICK_PERIOD_MS);  
  
       tx_msg->data[0] = 0x21;
       tx_msg->data[1] = 0x5A;
       tx_msg->data[2] = 0x5A;
       tx_msg->data[3] = 0x5A;
       tx_msg->data[4] = 0x39;
       tx_msg->data[5] = 0x39;
       tx_msg->data[6] = 0x5A;
       tx_msg->data[7] = 0x54;
      can_transmit(tx_msg, portMAX_DELAY);
      vTaskDelay(5 / portTICK_PERIOD_MS);
  
  
       tx_msg->data[0] = 0x22;
       tx_msg->data[1] = 0x53;
       tx_msg->data[2] = 0x33;
       tx_msg->data[3] = 0x39;
       tx_msg->data[4] = 0x32;
       tx_msg->data[5] = 0x31;
       tx_msg->data[6] = 0x32;
       tx_msg->data[7] = 0x34;
      can_transmit(tx_msg, portMAX_DELAY);
       
  
  #endif
    
#ifdef OBD_VIN
      
      ESP_LOGI(CAN_TAG, "OBD VIN  ");
                
       tx_msg->identifier =  0x7E8;
       tx_msg->data_length_code = 8;
       tx_msg->flags = CAN_MSG_FLAG_NONE;
       tx_msg->data[0] = 0x10;
       tx_msg->data[1] = 0x22;                               
       tx_msg->data[2] = 0x49;
       tx_msg->data[3] = 0x02;
       tx_msg->data[4] = 0x01;
       tx_msg->data[5] = 0x00;
       tx_msg->data[6] = 0x00;
       tx_msg->data[7] = 0x00;
       
        can_transmit(tx_msg, portMAX_DELAY);
        vTaskDelay(10 / portTICK_PERIOD_MS);
        
       tx_msg->identifier =  0x7E8;
       tx_msg->data_length_code = 8;
       tx_msg->flags = CAN_MSG_FLAG_NONE;
       tx_msg->data[0] = 0x21;
       tx_msg->data[1] = 0x49;                               
       tx_msg->data[2] = 0x02;
       tx_msg->data[3] = 0x02;
       tx_msg->data[4] = 0x47;
       tx_msg->data[5] = 0x31;
       tx_msg->data[6] = 0x4A;
       tx_msg->data[7] = 0x43;
       
       can_transmit(tx_msg, portMAX_DELAY);
       vTaskDelay(5 / portTICK_PERIOD_MS);
        
      tx_msg->identifier =  0x7E8;
       tx_msg->data_length_code = 8;
       tx_msg->flags = CAN_MSG_FLAG_NONE;
       tx_msg->data[0] = 0x22;
       tx_msg->data[1] = 0x49;                               
       tx_msg->data[2] = 0x02;
       tx_msg->data[3] = 0x03;
       tx_msg->data[4] = 0x35;
       tx_msg->data[5] = 0x34;
       tx_msg->data[6] = 0x34;
       tx_msg->data[7] = 0x34;
       
        can_transmit(tx_msg, portMAX_DELAY);
         vTaskDelay(5 / portTICK_PERIOD_MS);
        
        tx_msg->identifier =  0x7E8;
       tx_msg->data_length_code = 8;
       tx_msg->flags = CAN_MSG_FLAG_NONE;
       tx_msg->data[0] = 0x23;
       tx_msg->data[1] = 0x49;                               
       tx_msg->data[2] = 0x02;
       tx_msg->data[3] = 0x04;
       tx_msg->data[4] = 0x52;
       tx_msg->data[5] = 0x37;
       tx_msg->data[6] = 0x32;
       tx_msg->data[7] = 0x35;
       
        can_transmit(tx_msg, portMAX_DELAY);
         vTaskDelay(5 / portTICK_PERIOD_MS);
         
       tx_msg->identifier =  0x7E8;
       tx_msg->data_length_code = 8;
       tx_msg->flags = CAN_MSG_FLAG_NONE;
       tx_msg->data[0] = 0x24;
       tx_msg->data[1] = 0x49;                               
       tx_msg->data[2] = 0x02;
       tx_msg->data[3] = 0x05;
       tx_msg->data[4] = 0x32;
       tx_msg->data[5] = 0x33;
       tx_msg->data[6] = 0x36;
       tx_msg->data[7] = 0x37;
       
        can_transmit(tx_msg, portMAX_DELAY);
                                            
         vTaskDelay(5 / portTICK_PERIOD_MS);
       
#endif
      ESP_LOGI(CAN_TAG, "VIN transmitted ");
    }
   
   
   }
#endif
}
 
 
 void setFakeRPM(void)
 {
       bufRPM.identifier =  0x7E8;
       bufRPM.data_length_code = 8;
       bufRPM.flags = CAN_MSG_FLAG_NONE;
       bufRPM.data[0] = 0x04;
       bufRPM.data[1] = 0x41;                               
       bufRPM.data[2] = 0x0C;
       bufRPM.data[3] = 0x0F;
       bufRPM.data[4] = 0xF0;
       bufRPM.data[5] = 0x00;
       bufRPM.data[6] = 0x00;
       bufRPM.data[7] = 0x00;
 
 }
 
 void sendRPM(can_message_t *tx )
 {
    tx->identifier = bufRPM.identifier;
       tx->data_length_code = bufRPM.data_length_code ;
       tx->flags = bufRPM.flags ;
       tx->data[0] = bufRPM.data[0] ;
       tx->data[1] = bufRPM.data[1];                               
       tx->data[2] = bufRPM.data[2];
       tx->data[3] = bufRPM.data[3];
       tx->data[4] = bufRPM.data[4];
       tx->data[5] = bufRPM.data[5];
       tx->data[6] = bufRPM.data[6];
       tx->data[7] = bufRPM.data[7];
 
 }
 
 void storeRPM(can_message_t *rx )
 {
       bufRPM.identifier =  rx->identifier;
       bufRPM.data_length_code = rx->data_length_code;
       bufRPM.flags = rx->flags;
       bufRPM.data[0] = rx->data[0];
       bufRPM.data[1] = rx->data[1];                               
       bufRPM.data[2] = rx->data[2];
       bufRPM.data[3] = rx->data[3];
       bufRPM.data[4] = rx->data[4];
       bufRPM.data[5] = rx->data[5];
       bufRPM.data[6] = rx->data[6];
       bufRPM.data[7] = rx->data[7];
 
      fakeRPMflag = 0;
 }