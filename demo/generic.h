/**
 *
 * Microvisor C Demos
 *
 * Copyright © 2024, KORE Wireless
 * Licence: MIT
 *
 */
#ifndef _GENERIC_H_
#define _GENERIC_H_


#ifdef __cplusplus
extern "C" {
#endif


/*
 * PROTOTYPES
 */
void system_clock_config(void);
void show_wake_reason(void);
void log_device_info(void);
void control_system_led(bool do_enable);


#ifdef __cplusplus
}
#endif


#endif  // _GENERIC_H_