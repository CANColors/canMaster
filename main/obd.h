
typedef enum
{
  OBD_REQUEST_NONE,
  OBD_REQUEST_NEED_SEND, 
  OBD_REQUEST_WAIT_FOR_RESPONSE

}OBD_Request_States;

typedef struct 
{
    uint8_t service;
    uint8_t pid;
    uint8_t resLen;
    uint8_t* response;
    OBD_Request_States requestState;
}OBDPid;

typedef enum
{
  OBD_RESPONSE_NONE, 
  OBD_RESPONSE_NEED_TO_SEND,
  OBS_RESPONSE_ERROR_NETWORK
} OBDResponseStatus;

void obd_wait_task(void *arg);
void OBDInit(void);
OBDResponseStatus OBDResponse (can_message_t *rx_msg);
OBDResponseStatus  OBDSend (can_msg_timestamped *msg_timestamped);