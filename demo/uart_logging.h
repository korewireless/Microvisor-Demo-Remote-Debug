/**
 *
 * Microvisor C Demos
 *
 * Copyright © 2024, KORE Wireless
 * Licence: MIT
 *
 */
#ifndef UART_LOGGING_H
#define UART_LOGGING_H


/*
 * CONSTANTS
 */
#define UART_LOG_TIMESTAMP_MAX_LEN_B        64
#define UART_LOG_MESSAGE_MAX_LEN_B          1024


#ifdef __cplusplus
extern "C" {
#endif


/*
 * PROTOTYPES
 */
bool    log_uart_init(void);
void    log_uart_output(const char* buffer);


#ifdef __cplusplus
}
#endif


#endif
