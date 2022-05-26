/**
 *
 * Microvisor Remote Debugging Demo
 * Version 1.0.0
 * Copyright © 2022, Twilio
 * Licence: Apache 2.0
 *
 */
#ifndef _MAIN_H_
#define _MAIN_H_


/*
 * INCLUDES
 */
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

// Microvisor includes
#include "stm32u5xx_hal.h"
#include "mv_syscalls.h"

// App includes
#include "logging.h"


#ifdef __cplusplus
extern "C" {
#endif


/*
 * CONSTANTS
 */
#define     LED_GPIO_BANK               GPIOA
#define     LED_GPIO_PIN                GPIO_PIN_5

#define     REQUEST_SEND_PERIOD_US      30000 * 1000
#define     CHANNEL_KILL_PERIOD_US      15000 * 1000
#define     LED_FLASH_PERIOD_US         250 * 1000

#define     HTTP_RX_BUFFER_SIZE_B       1536
#define     HTTP_TX_BUFFER_SIZE_B       512


/*
 * PROTOTYPES
 */
void        gpio_init(void);

void        http_channel_center_setup(void);
bool        http_open_channel(void);
void        http_close_channel(void);
bool        http_send_request();
void        http_process_response(void);

void        log_device_info(void);
void        server_log(char* format_string, ...) ;
void        server_error(char* format_string, ...);

void        debug_function_parent(uint32_t* vptr);
bool        debug_function_child(uint32_t* vptr);


#ifdef __cplusplus
}
#endif


#endif      // _MAIN_H_
