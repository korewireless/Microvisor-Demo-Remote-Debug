/**
 *
 * Microvisor Remote Debugging Demo
 * Version 3.0.2
 * Copyright © 2023, Twilio
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
#include "uart_logging.h"
#include "http.h"
#include "network.h"


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


/*
 * PROTOTYPES
 */
void        debug_function_parent(uint32_t* vptr);
bool        debug_function_child(uint32_t* vptr);


#ifdef __cplusplus
}
#endif


#endif      // _MAIN_H_
