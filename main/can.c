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
#include "obd.h"

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
     OBDResponseStatus status = OBD_RESPONSE_NONE;
   // 
   // testCanDataGenerate();
     ESP_LOGI(CAN_TAG, "Wait for CAN");    
    while (1) {                                                                                                  
            
        can_message_t rx_msg;
        esp_err_t res = can_receive(&rx_msg, portMAX_DELAY);
        if (res == ESP_OK)
        { 
         ESP_LOGI(CAN_TAG, "REceived");
          status =  OBDResponse (&rx_msg); 
  
         if (status == OBD_RESPONSE_NEED_TO_SEND)
          {
            can_msg_timestamped msg_timestamped;
            msg_timestamped.id = cntRxCan++;
            msg_timestamped.timestamp = get_timestamp();
            msg_timestamped.msg.identifier = rx_msg.identifier;
            msg_timestamped.msg.data_length_code = rx_msg.data_length_code;
            for (int i=0; i<rx_msg.data_length_code; i++)
            {
              msg_timestamped.msg.data[i] = rx_msg.data[i];
            }
    
                                        
        
            ControlEvents cs2 = EV_CAN_RECEIVED;    
            xQueueSend(controlEvents, &cs2, portMAX_DELAY);
            xQueueSend(rxCanQueue, &msg_timestamped, portMAX_DELAY);
          }
             
           
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
          OBDSend (&msg_timestamped);
       //  ESP_LOGI(CAN_TAG, "Send to CAN");   
       // msg_timestamped.msg.flags = CAN_MSG_FLAG_NONE;
       // can_transmit(&msg_timestamped.msg, portMAX_DELAY);
             
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

   
  //  xSemaphoreGive(rx_sem);                     //Start RX task
}

