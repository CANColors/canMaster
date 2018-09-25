/* ========================================================================== */
/*                                                                            */
/*   Filename.c                                                               */
/*   (c) 2012 Author                                                          */
/*                                                                            */
/*   Description                                                              */
/*                                                                            */
/* ========================================================================== */
void post( const char* page, const char *post_data);
void http_receive_task(void *arg);
void http_transmit_task(void *arg);
void http_wait_task(void *arg);