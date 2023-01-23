/**
 *
 * Microvisor Remote Debugging Demo
 * Version 2.0.6
 * Copyright © 2023, Twilio
 * Licence: Apache 2.0
 *
 */
#include "main.h"


/*
 * STATIC PROTOTYPES
 */
static void net_open_network(void);
static void net_notification_center_setup(void);
static void log_start(void);
static void log_service_setup(void);
static void post_log(bool is_err, char* format_string, va_list args);


/*
 * GLOBALS
 */
// Central store for Microvisor resource handles used in this code.
// See 'https://www.twilio.com/docs/iot/microvisor/syscalls#handles'
static struct {
    MvNotificationHandle notification;
    MvNetworkHandle      network;
    uint32_t             log;
} logging_handles = { 0, 0, 0 };

// Central store for network management notification records.
// Holds eight records at a time -- each record is 16 bytes in size.
static volatile struct MvNotification net_notification_buffer[8] __attribute__((aligned(8)));
static volatile uint32_t current_notification_idx = 0;

// Entities for Microvisor application logging
static const uint32_t log_buffer_size = 4096;
static uint8_t log_buffer[4096] __attribute__((aligned(512))) = {0} ;

// Entities for local serial logging
// Declared in `uart_logging.c`
extern UART_HandleTypeDef uart;
static bool uart_available = false;


/**
 * @brief  Open a logging channel.
 *
 * Open a data channel for Microvisor logging.
 * This call will also request a network connection.
 */
static void log_start(void) {
    
    // Initiate the Microvisor logging service
    log_service_setup();

    // Connect to the network
    // NOTE This connection spans logging and HTTP comms
    net_open_network();

#if ENABLE_UART_DEBUGGING == true
    // Establish UART logging
    uart_available = log_uart_init();
#endif
}


/**
 * @brief Initiate Microvisor application logging.
 */
static void log_service_setup(void) {
    
    if (logging_handles.log != USER_HANDLE_LOGGING_STARTED) {
        // Initialize logging with the standard system call
        enum MvStatus status = mvServerLoggingInit(log_buffer, log_buffer_size);

        // Set a mock handle as a proxy for a 'logging enabled' flag
        if (status == MV_STATUS_OKAY) logging_handles.log = USER_HANDLE_LOGGING_STARTED;
        do_assert(status == MV_STATUS_OKAY, "Could not start logging");
    }
}


/**
 * @brief Configure and connect to the network.
 */
static void net_open_network(void) {
    
    // Configure the network's notification center
    net_notification_center_setup();

    if (logging_handles.network == 0) {
        // Configure the network connection request
        struct MvRequestNetworkParams network_config = {
            .version = 1,
            .v1 = {
                .notification_handle = logging_handles.notification,
                .notification_tag = USER_TAG_LOGGING_REQUEST_NETWORK,
            }
        };

        // Ask Microvisor to establish the network connection
        // and confirm that it has accepted the request
        enum MvStatus status = mvRequestNetwork(&network_config, &logging_handles.network);
        do_assert(status == MV_STATUS_OKAY, "Could not open network");

        // The network connection is established by Microvisor asynchronously,
        // so we wait for it to come up before opening the data channel -- which
        // would fail otherwise
        enum MvNetworkStatus net_status;
        while (1) {
            // Request the status of the network connection, identified by its handle.
            // If we're good to continue, break out of the loop...
            if (mvGetNetworkStatus(logging_handles.network, &net_status) == MV_STATUS_OKAY && net_status == MV_NETWORKSTATUS_CONNECTED) {
                break;
            }

            // ... or wait a short period before retrying
            for (volatile unsigned i = 0; i < 50000; ++i) {
                // No op
                __asm("nop");
            }
        }
    }
}


/**
 * @brief Configure the network Notification Center.
 */
static void net_notification_center_setup(void) {
    
    if (logging_handles.notification == 0) {
        // Clear the notification store
        memset((void *)net_notification_buffer, 0xff, sizeof(net_notification_buffer));

        // Configure a notification center for network-centric notifications
        static struct MvNotificationSetup net_notification_config = {
            .irq = TIM1_BRK_IRQn,
            .buffer = (struct MvNotification *)net_notification_buffer,
            .buffer_size = sizeof(net_notification_buffer)
        };

        // Ask Microvisor to establish the notification center
        // and confirm that it has accepted the request
        enum MvStatus status = mvSetupNotifications(&net_notification_config, &logging_handles.notification);
        do_assert(status == MV_STATUS_OKAY, "Could not start network NC");

        // Start the notification IRQ
        NVIC_ClearPendingIRQ(TIM1_BRK_IRQn);
        NVIC_EnableIRQ(TIM1_BRK_IRQn);
        server_log("Network NC handle: %lu", (uint32_t)logging_handles.notification);
    }
}


/**
 * @brief Issue a debug message.
 *
 * @param format_string Message string with optional formatting
 * @param ...           Optional injectable values
 */
void server_log(char* format_string, ...) {
    
    if (LOG_DEBUG_MESSAGES) {
        va_list args;
        va_start(args, format_string);
        post_log(false, format_string, args);
        va_end(args);
    }
}


/**
 * @brief Issue an error message.
 *
 * @param format_string Message string with optional formatting
 * @param ...           Optional injectable values
 */
void server_error(char* format_string, ...) {
    
    va_list args;
    va_start(args, format_string);
    post_log(true, format_string, args);
    va_end(args);
}


/**
 * @brief Issue any log message.
 *
 * @param is_err        Is the message an error?
 * @param format_string Message string with optional formatting
 * @param args          va_list of args from previous call
 */
static void post_log(bool is_err, char* format_string, va_list args) {
    
    if (get_net_handle() == 0) log_start();
    char buffer[LOG_MESSAGE_MAX_LEN_B] = {0};

    // Write the message type to the message
    sprintf(buffer, is_err ? "[ERROR] " : "[DEBUG] ");

    // Write the formatted text to the message
    vsnprintf(&buffer[8], sizeof(buffer) - 9, format_string, args);

    // Output the message using the system call
    mvServerLog((const uint8_t*)buffer, (uint16_t)strlen(buffer));

    // Do we output via UART too?
    if (uart_available) log_uart_output(buffer);
}


/**
 * @brief Provide the current network handle.
 *
 * @returns The network handle.
 */
MvNetworkHandle get_net_handle(void) {
    
    return logging_handles.network;
}


/**
 * @brief Provide the current logging handle.
 *
 * @returns The logging handle.
 */
uint32_t get_log_handle(void) {
    
    return logging_handles.log;
}


/**
 * @brief Network notification ISR.
 */
void TIM1_BRK_IRQHandler(void) {
    
    // Network notifications interrupt service handler
    // Add your own notification processing code here
}


/**
 * @brief Wrapper for asserts so we get log output on fail.
 *
 * @param condition The condition to check.
 * @param message   The error message.
 */
void do_assert(bool condition, char* message) {
    
    if (!condition) {
        server_error(message);
        assert(false);
    }
}
