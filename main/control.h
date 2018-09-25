/* ========================================================================== */
/*                                                                            */
/*   Filename.c                                                               */
/*   (c) 2012 Author                                                          */
/*                                                                            */
/*   Description                                                              */
/*                                                                            */
/* ========================================================================== */
#define CONTROL_QUEUE_MAX   10

typedef enum
{
   CONTROL_STOP,
   CONTROL_START,
   CONTROL_CAN_REQUEST_WAIT,
   CONTROL_SEND_REQUEST,
   CONTROL_WAIT_RESPONSE,
   CONTROL_RESPONSE_TIMEOUT,
   CONTROL_RESPONSE_TRANSMITTED
      
}ControlState;

typedef enum
{
  EV_START,
  EV_STOP, 
  EV_CAN_REQUEST,
  EV_HTTP_START,
  EV_HTTP_END,
  EV_HTTP_TIMEOUT,
  EV_CAN_TRANSMITTED,
  EV_BIG_TIMEOUT
  
}ControlEvents;

void control_task(void *arg);