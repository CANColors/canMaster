/* ========================================================================== */
/*                                                                            */
/*   Filename.c                                                               */
/*   (c) 2012 Author                                                          */
/*                                                                            */
/*   Description                                                              */
/*                                                                            */
/* ========================================================================== */

typedef enum
{
  ISOSTATUS_NONE, 
  ISOSTATUS_OK, 
  ISOSTATUS_FAILED

} ISOStatus;


ISOStatus iso_receiveMF(can_message_t* rx_msg, uint8_t* data,  uint8_t len);
ISOStatus iso_send(uint8_t* data,  uint8_t len);