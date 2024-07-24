/**
 *
 * Microvisor C Demos
 *
 * Copyright © 2024, KORE Wireless
 * Licence: MIT
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
#define     USER_HANDLE_LOGGING_OFF             0

#define     LOG_MESSAGE_MAX_LEN_B               1024
#define     LOG_BUFFER_SIZE_B                   8192


#ifdef __cplusplus
extern "C" {
#endif


/*
 * PROTOTYPES
 */
void server_log(const char* format_string, ...)        __attribute__ ((__format__ (__printf__, 1, 2)));
void server_error(const char* format_string, ...)      __attribute__ ((__format__ (__printf__, 1, 2)));
void do_assert(bool condition, const char* message);


#ifdef __cplusplus
}
#endif


#endif /* LOGGING_H */
