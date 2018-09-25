/* ========================================================================== */
/*                                                                            */
/*   Filename.c                                                               */
/*   (c) 2012 Author                                                          */
/*                                                                            */
/*   Description                                                              */
/*                                                                            */
/* ========================================================================== */
#include <cjson.h>

//void request_login();
char* request_login();
char* request_server_masterrequest(void);
char* request_heartbeat(void);
void request_can_send (CJSON_PUBLIC(cJSON *)jdata);
char* request_response(void);
