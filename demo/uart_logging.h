/**
 *
 * Microvisor Remote Debugging Demo
 * Version 3.0.3
 * Copyright © 2023, Twilio
 * Licence: MIT
 *
 */
#ifndef UART_LOGGING_H
#define UART_LOGGING_H


#ifdef __cplusplus
extern "C" {
#endif


/*
 * CONSTANTS
 */
#define UART_LOG_TIMESTAMP_MAX_LEN_B        64
#define UART_LOG_MESSAGE_MAX_LEN_B          64


/*
 * PROTOTYPES
 */
bool    log_uart_init(void);
void    log_uart_output(char* buffer);


#ifdef __cplusplus
}
#endif


#endif
