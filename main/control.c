/* ========================================================================== */
/*                                                                            */
/*   Filename.c                                                               */
/*   (c) 2012 Author                                                          */
/*                                                                            */
/*   Description                                                              */
/*                                                                            */
/* ========================================================================== */
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <esp_log.h>

#include "control.h"



extern SemaphoreHandle_t can_rx;
extern SemaphoreHandle_t can_tx;
extern SemaphoreHandle_t net_rx;
extern SemaphoreHandle_t net_tx;
extern SemaphoreHandle_t obd_tx_wait;

static const char *CONTROL_TAG="CONTROL";
ControlState cs = CONTROL_START;
ControlState prevState = CONTROL_START;
QueueHandle_t controlEvents;
   
void controlNetResponseWaitState(ControlEvents ev);
void controlNetRequestSendState(ControlEvents ev);
void controlStartState(ControlEvents ev);
void controlCANRequestState(ControlEvents ev);

static void changeState(ControlState newState);


void control_task(void *arg)
{
   ControlEvents ev;
   
   controlEvents = xQueueCreate( CONTROL_QUEUE_MAX, sizeof( ControlEvents) );
   ControlEvents cs2 = EV_START;    //return to start
   xQueueSend(controlEvents, &cs2, portMAX_DELAY);
   
  while(1)
  {
     xQueueReceive(controlEvents, &ev, portMAX_DELAY);
     switch(cs)
     {
          case CONTROL_START:
          {
              ESP_LOGI(CONTROL_TAG, "CONTROL_START" );
              controlStartState(ev);
              break;
          }
          
          case CONTROL_CAN_REQUEST_WAIT:
          {
              ESP_LOGI(CONTROL_TAG, "CONTROL_CAN_REQUEST_WAIT" );
               controlCANRequestState(ev);
              break;
          }
          
          case CONTROL_SEND_REQUEST:
          {
              ESP_LOGI(CONTROL_TAG, "CONTROL_SEND_REQUEST" );
              controlNetRequestSendState(ev);
              break;
          }  
                   
          case CONTROL_WAIT_RESPONSE:
          {
           ESP_LOGI(CONTROL_TAG, "CONTROL_WAIT_RESPONSE" );
              controlNetResponseWaitState(ev);
               break;
          
          }
          case CONTROL_RESPONSE_TRANSMITTED:
          {
           ESP_LOGI(CONTROL_TAG, "CONTROL_RESPONSE_TRANSMITTED" );
       
           changeState (CONTROL_CAN_REQUEST_WAIT);
                      
           ControlEvents cs2 = EV_START;    
           xQueueSend(controlEvents, &cs2, portMAX_DELAY);
       
            break;
          }
          case CONTROL_STOP: { break; }
          case CONTROL_RESPONSE_TIMEOUT: { break; }
          default:break;
     }
  }

}


void controlStartState(ControlEvents ev)
{
  switch (ev)
  {
    case EV_START:
    {
        //Wait for data from CAN bus 
        xSemaphoreGive(can_rx);
        changeState(CONTROL_CAN_REQUEST_WAIT);
         break;
    }
  
   case EV_STOP: { break; }
   case EV_CAN_RECEIVED: { break;}
   case EV_NET_TRANSMIT_START:{break;}
   case EV_NET_TRANSMIT_END:{break;}
   case EV_NET_RECEIVED:{break;}
   case EV_NET_TRANSMIT_TIMEOUT: {break;}
   case EV_BIG_TIMEOUT: {break;}
    default: break;
  }
}

void controlCANRequestState(ControlEvents ev)
{
  switch (ev)
  {
   case EV_START:{break;}
   case EV_STOP: { break; }
   case EV_CAN_RECEIVED: 
   { 
      xSemaphoreGive(net_tx); //  sending the request
     // xSemaphoreGive(obd_tx_wait);
      changeState(CONTROL_SEND_REQUEST);
     
    break;
   }
   case EV_NET_TRANSMIT_START:{break;}
   case EV_NET_TRANSMIT_END:{break;}
   case EV_NET_TRANSMIT_TIMEOUT: {break;}
   case EV_NET_RECEIVED:{break;}
   case EV_BIG_TIMEOUT: {break;}
   default: break;
  }
}

void controlNetRequestSendState(ControlEvents ev)
{
  switch (ev)
  {
   case EV_START:{break;}
   case EV_STOP: { break; }
   case EV_CAN_RECEIVED: { break;}
   case EV_NET_TRANSMIT_START:{break;}
   case EV_NET_TRANSMIT_END:
   {
      xSemaphoreGive(net_rx);   //Looking for the response
     changeState(CONTROL_WAIT_RESPONSE);
      
    break;
   }
   case EV_NET_TRANSMIT_TIMEOUT: {break;}
   case EV_NET_RECEIVED:{break;}
   case EV_BIG_TIMEOUT:
    {
      changeState(CONTROL_START);
       ControlEvents cs2 = EV_START;    
       xQueueSend(controlEvents, &cs2, portMAX_DELAY);
       
      //xSemaphoreGive(obd_tx_wait); // Wait again
      break;
    }
   default: break;
  }
}

void controlNetResponseWaitState(ControlEvents ev)
{
  switch (ev)
  {
   case EV_START:{break;}
   case EV_STOP: { break; }
   case EV_CAN_RECEIVED: { break;}
   case EV_NET_TRANSMIT_START: { break; }
   case EV_NET_TRANSMIT_END: { break; }
   case EV_NET_RECEIVED:{
     xSemaphoreGive(can_tx);
    changeState(CONTROL_RESPONSE_TRANSMITTED);
    
   break;
   }
   
   case EV_NET_TRANSMIT_TIMEOUT:{ break; }
   case EV_BIG_TIMEOUT: 
   {
       changeState(CONTROL_START);
       ControlEvents cs2 = EV_START;    
       xQueueSend(controlEvents, &cs2, portMAX_DELAY);
      
    //xSemaphoreGive(obd_tx_wait); // Wait again
    break;
  }
   default: break;
  }
}

/*****************************************************************************/
/*         change state                                                      */
/*****************************************************************************/
void changeState(ControlState newState)
{
  prevState = cs;
  cs = newState;

}


/*
void controlHTTPRequestSendState(ControlEvents ev)
{
  switch (ev)
  {
   case EV_START:{break;}
   case EV_STOP: { break; }
   case EV_CAN_RECEIVED: { break;}
   case EV_NET_REQUEST_START:{break;}
   case EV_NET_REQUEST_END:{break;}
   case EV_NET_REQUEST_TIMEOUT: {break;}
   case EV_NET_RESPONSE_START:{break;}
   case EV_NET_RESPONSE_END:{break;}
   case EV_NET_RESPONSE_TIMEOUT: {break;}
   default: break;
  }
}
 */
 
 