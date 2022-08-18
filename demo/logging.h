/**
 *
 * Microvisor Remote Debugging Demo
 * Version 1.0.4
 * Copyright © 2022, Twilio
 * Licence: Apache 2.0
 *
 */
#ifndef LOGGING_H
#define LOGGING_H


/*
 * CONSTANTS
 */
#define     USER_TAG_LOGGING_REQUEST_NETWORK    1
#define     USER_TAG_LOGGING_OPEN_CHANNEL       2
#define     USER_TAG_HTTP_OPEN_CHANNEL          3


#ifdef __cplusplus
extern "C" {
#endif


/*
 * PROTOTYPES
 */
void            log_open_channel(void);
void            log_close_channel(void);
void            log_channel_center_setup(void);
void            log_open_network(void);
MvNetworkHandle get_net_handle();


#ifdef __cplusplus
}
#endif


#endif /* LOGGING_H */
