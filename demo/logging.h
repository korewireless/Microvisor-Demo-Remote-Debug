/**
 *
 * Microvisor Remote Debugging Demo
 * Version 2.0.6
 * Copyright © 2023, Twilio
 * Licence: Apache 2.0
 *
 */
#ifndef LOGGING_H
#define LOGGING_H


#ifdef __cplusplus
extern "C" {
#endif


/*
 * CONSTANTS
 */
#define     USER_TAG_LOGGING_REQUEST_NETWORK    1
#define     USER_TAG_LOGGING_OPEN_CHANNEL       2
#define     USER_TAG_HTTP_OPEN_CHANNEL          3

#define     USER_HANDLE_LOGGING_STARTED         0xFFFF

#define     LOG_MESSAGE_MAX_LEN_B               1024


/*
 * PROTOTYPES
 */
MvNetworkHandle get_net_handle(void);
uint32_t        get_log_handle(void);

void            server_log(char* format_string, ...);
void            server_error(char* format_string, ...);
void            do_assert(bool condition, char* message);


#ifdef __cplusplus
}
#endif


#endif /* LOGGING_H */
