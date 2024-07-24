/**
 *
 * Microvisor C Demos
 *
 * Copyright © 2024, KORE Wireless
 * Licence: MIT
 *
 */
#include "main.h"


/*
 * STATIC PROTOTYPES
 */
static void net_setup_notification_center(void);


/*
 * GLOBALS
 */
// Central store for Microvisor resource handles used in this code.
// See 'https://www.twilio.com/docs/iot/microvisor/syscalls#handles'
static struct {
    MvNotificationHandle notification;
    MvNetworkHandle      network;
} net_handles = { 0, 0 };

// Central store for network management notification records.
// Holds 'NET_NC_BUFFER_SIZE_R' records at a time -- each record is 16 bytes in size.
static struct MvNotification net_notification_buffer[NET_NC_BUFFER_SIZE_R] __attribute__((aligned(8)));


/**
 * @brief Configure and connect to the network.
 */
void net_open_network(void) {

    // Configure the network's notification center
    net_setup_notification_center();

    if (net_handles.network == 0) {
        // Configure the network connection request
        const struct MvRequestNetworkParams network_config = {
            .version = 1,
            .v1 = {
                .notification_handle = net_handles.notification,
                .notification_tag = USER_TAG_LOGGING_REQUEST_NETWORK,
            }
        };

        // Ask Microvisor to establish the network connection
        // and confirm that it has accepted the request
        enum MvStatus status = mvRequestNetwork(&network_config, &net_handles.network);
        do_assert(status == MV_STATUS_OKAY, "Could not open network");

        // The network connection is established by Microvisor asynchronously,
        // so we wait for it to come up before opening the data channel -- which
        // would fail otherwise
        enum MvNetworkStatus net_status;
        while (1) {
            // Request the status of the network connection, identified by its handle.
            // If we're good to continue, break out of the loop...
            if (mvGetNetworkStatus(net_handles.network, &net_status) == MV_STATUS_OKAY && net_status == MV_NETWORKSTATUS_CONNECTED) {
                break;
            }

            // ... or wait a short period before retrying
            for (size_t i = 0; i < 50000; ++i) {
                // No op
                __asm("nop");
            }
        }
    }
}


/**
 * @brief Configure the network Notification Center.
 */
static void net_setup_notification_center(void) {

    if (net_handles.notification == 0) {
        // Clear the notification store
        memset((void*)net_notification_buffer, 0xff, sizeof(net_notification_buffer));

        // Configure a notification center for network-centric notifications
        const struct MvNotificationSetup net_notification_config = {
            .irq = TIM2_IRQn,
            .buffer = (struct MvNotification*)net_notification_buffer,
            .buffer_size = sizeof(net_notification_buffer)
        };

        // Ask Microvisor to establish the notification center
        // and confirm that it has accepted the request
        enum MvStatus status = mvSetupNotifications(&net_notification_config, &net_handles.notification);
        do_assert(status == MV_STATUS_OKAY, "Could not start network Notification Center");

        // Start the notification IRQ
        NVIC_ClearPendingIRQ(TIM2_IRQn);
        NVIC_EnableIRQ(TIM2_IRQn);
        server_log("Network Notification Center handle: %lu", (uint32_t)net_handles.notification);
    }
}


/**
 * @brief Provide the current network handle.
 *
 * @returns The network handle.
 */
MvNetworkHandle net_get_handle(void) {

    return net_handles.network;
}


/**
 * @brief Network notification ISR.
 */
void TIM2_IRQHandler(void) {

    // Network notifications interrupt service handler
    // Add your own notification processing code here
}


