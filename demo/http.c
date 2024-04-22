/**
 *
 * Microvisor Remote Debugging Demo
 *
 * Copyright © 2024, KORE Wireless
 * Licence: MIT
 *
 */
#include "main.h"


/*
 * GLOBALS
 */
// Central store for Microvisor resource handles used in this code.
// See `https://www.twilio.com/docs/iot/microvisor/syscalls#handles`
struct {
    MvNotificationHandle notification;
    MvNetworkHandle      network;
    MvChannelHandle      channel;
} http_handles = { 0, 0, 0 };

// Central store for HTTP request management notification records.
// Holds HTTP_NT_BUFFER_SIZE_R records at a time -- each record is 16 bytes in size.
static volatile struct MvNotification http_notification_center[HTTP_NT_BUFFER_SIZE_R] __attribute__((aligned(8)));
static volatile uint32_t current_notification_index = 0;

// Defined in `main.c`
extern volatile bool received_request;
extern volatile bool channel_was_closed;


/**
 * @brief Open a new HTTP channel.
 *
 * @returns `true` if the channel is open, otherwise `false`.
 */
bool http_open_channel(void) {

    // Set up the HTTP channel's multi-use send and receive buffers
    static volatile uint8_t http_rx_buffer[HTTP_RX_BUFFER_SIZE_B] __attribute__((aligned(512)));
    static volatile uint8_t http_tx_buffer[HTTP_TX_BUFFER_SIZE_B] __attribute__((aligned(512)));

    // Get the network channel handle.
    // NOTE This is set in `logging.c` which puts the network in place
    //      (ie. so the network handle != 0) well in advance of this being called
    http_handles.network = net_get_handle();
    if (http_handles.network == 0) return false;
    server_log("Network handle: %lu", (uint32_t)http_handles.network);

    // Configure the required data channel
    struct MvOpenChannelParams channel_config = {
        .version = 1,
        .v1 = {
            .notification_handle = http_handles.notification,
            .notification_tag    = USER_TAG_HTTP_OPEN_CHANNEL,
            .network_handle      = http_handles.network,
            .receive_buffer      = (uint8_t*)http_rx_buffer,
            .receive_buffer_len  = sizeof(http_rx_buffer),
            .send_buffer         = (uint8_t*)http_tx_buffer,
            .send_buffer_len     = sizeof(http_tx_buffer),
            .channel_type        = MV_CHANNELTYPE_HTTP,
            .endpoint            = {
                .data = (uint8_t*)"",
                .length = 0
            }
        }
    };

    // Ask Microvisor to open the channel
    // and confirm that it has accepted the request
    enum MvStatus status = mvOpenChannel(&channel_config, &http_handles.channel);
    if (status == MV_STATUS_OKAY) {
        server_log("HTTP channel handle: %lu", (uint32_t)http_handles.channel);
        return true;
    }

    server_error("Could not open HTTP channel. Status: %i", status);
    return false;
}


/**
 * @brief Close the currently open HTTP channel.
 */
void http_close_channel(void) {

    // If we have a valid channel handle -- ie. it is non-zero --
    // then ask Microvisor to close it and confirm acceptance of
    // the closure request.
    if (http_handles.channel != 0) {
        MvChannelHandle old = http_handles.channel;
        enum MvStatus status = mvCloseChannel(&http_handles.channel);
        do_assert((status == MV_STATUS_OKAY || status == MV_STATUS_CHANNELCLOSED), "Channel closure");
        server_log("HTTP channel %lu closed (status code: %i)", (uint32_t)old, status);
    }

    // Confirm the channel handle has been invalidated by Microvisor
    do_assert(http_handles.channel == 0, "Channel handle not zero");
}


/**
 * @brief Configure the channel Notification Center.
 */
void http_setup_notification_center(void) {

    // Clear the notification store
    memset((void *)http_notification_center, 0x00, sizeof(http_notification_center));

    // Configure a notification center for network-centric notifications
    static struct MvNotificationSetup http_notification_setup = {
        .irq = TIM8_BRK_IRQn,
        .buffer = (struct MvNotification *)http_notification_center,
        .buffer_size = sizeof(http_notification_center)
    };

    // Ask Microvisor to establish the notification center
    // and confirm that it has accepted the request
    enum MvStatus status = mvSetupNotifications(&http_notification_setup, &http_handles.notification);
    do_assert(status == MV_STATUS_OKAY, "Could not set up HTTP channel NC");

    // Start the notification IRQ
    NVIC_ClearPendingIRQ(TIM8_BRK_IRQn);
    NVIC_EnableIRQ(TIM8_BRK_IRQn);
    server_log("HTTP NC handle: %lu", (uint32_t)http_handles.notification);
}


/**
 * @brief Send a stock HTTP request.
 *
 * @returns `true` if the request was accepted by Microvisor, otherwise `false`
 */
enum MvStatus http_send_request(void) {

    static uint32_t item_number = 0;

    // Make sure we have a valid channel handle
    if (http_handles.channel == 0) {
        // There's no open channel, so open
        // one now and try to send again
        http_open_channel();
        return http_send_request();
    }

    server_log("Sending HTTP request");

    // Set up the request
    const char verb[] = "GET";
    const char body[] = "";
    char uri[46] = "";
    sprintf(uri, "https://jsonplaceholder.typicode.com/todos/%lu", item_number++);
    struct MvHttpHeader hdrs[] = {};
    struct MvHttpRequest request_config = {
        .method = {
            .data = (uint8_t *)verb,
            .length = strlen(verb)
        },
        .url = {
            .data = (uint8_t *)uri,
            .length = strlen(uri)
        },
        .num_headers = 0,
        .headers = hdrs,
        .body = {
            .data = (uint8_t *)body,
            .length = strlen(body)
        },
        .timeout_ms = 10000
    };

    // Issue the request -- and check its status
    enum MvStatus status = mvSendHttpRequest(http_handles.channel, &request_config);
    if (status == MV_STATUS_OKAY) {
        server_log("Request sent to Twilio");
    } else if (status == MV_STATUS_CHANNELCLOSED) {
        server_error("HTTP channel %lu already closed", (uint32_t)http_handles.channel);
    } else {
        server_error("Could not issue request. Status: %i", status);
    }

    return status;
}


/**
 * @brief The HTTP channel notification interrupt handler.
 *
 * This is called by Microvisor -- we need to check for key events
 * and extract HTTP response data when it is available.
 */
void TIM8_BRK_IRQHandler(void) {

    // Check for a suitable event: readable data in the channel
    bool got_notification = false;
    volatile struct MvNotification notification = http_notification_center[current_notification_index];
    if (notification.event_type == MV_EVENTTYPE_CHANNELDATAREADABLE) {
        // Flag we need to access received data and to close the HTTP channel
        // when we're back in the main loop. This lets us exit the ISR quickly.
        // We should not make Microvisor System Calls in the ISR.
        received_request = true;
        got_notification = true;
    }

    if (notification.event_type == MV_EVENTTYPE_CHANNELNOTCONNECTED) {
        // The HTTP channel signaled its unexpected closure
        channel_was_closed = true;
        got_notification = true;
    }

    if (got_notification) {
        // Point to the next record to be written
        current_notification_index = (current_notification_index + 1) % HTTP_NT_BUFFER_SIZE_R;

        // Clear the current notifications event
        // See https://www.twilio.com/docs/iot/microvisor/microvisor-notifications#buffer-overruns
        notification.event_type = 0;
    }
 }
