/**
 *
 * Microvisor Remote Debugging Demo
 * Version 2.0.2
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

#define     USER_HANDLE_LOGGING_STARTED         0xFFFF


#ifdef __cplusplus
extern "C" {
#endif


/*
 * PROTOTYPES
 */
void            log_start(void);
void            log_service_setup(void);

void            net_open_network(void);
void            net_notification_center_setup(void);

MvNetworkHandle get_net_handle(void);
uint32_t        get_log_handle(void);

void            server_log(char* format_string, ...);
void            server_error(char* format_string, ...);
void            do_log(bool is_err, char* format_string, va_list args);



#ifdef __cplusplus
}
#endif


#endif /* LOGGING_H */
