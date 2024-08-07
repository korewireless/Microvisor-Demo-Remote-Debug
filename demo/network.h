/**
 *
 * Microvisor C Demos
 *
 * Copyright © 2024, KORE Wireless
 * Licence: MIT
 *
 */
#ifndef _NETWORK_H_
#define _NETWORK_H_


/*
 * CONSTANTS
 */
#define     NET_NC_BUFFER_SIZE_R                8


#ifdef __cplusplus
extern "C" {
#endif


/*
 * PROTOTYPES
 */
void            net_open_network(void);
MvNetworkHandle net_get_handle(void);


#ifdef __cplusplus
}
#endif


#endif      // _NETWORK_H_
