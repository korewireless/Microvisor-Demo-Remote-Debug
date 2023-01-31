/**
 *
 * Microvisor HTTP Communications Demo
 * Version 2.0.8
 * Copyright © 2023, Twilio
 * Licence: Apache 2.0
 *
 */
#ifndef _HTTP_H_
#define _HTTP_H_


#ifdef __cplusplus
extern "C" {
#endif


/*
 * PROTOTYPES
 */
void            http_notification_center_setup(void);
bool            http_open_channel(void);
void            http_close_channel(void);
enum MvStatus   http_send_request(void);


#ifdef __cplusplus
}
#endif


#endif      // _HTTP_H_
