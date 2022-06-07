/**
 *
 * Microvisor Remote Debugging Demo
 * Version 1.0.2
 * Copyright © 2022, Twilio
 * Licence: Apache 2.0
 *
 */
#include "main.h"


/**
 *  GLOBALS
 */

// Central store for Microvisor resource handles used in this code.
// See `https://www.twilio.com/docs/iot/microvisor/syscalls#http_handles`
struct {
    MvNotificationHandle notification;
    MvNetworkHandle      network;
    MvChannelHandle      channel;
} http_handles = { 0, 0, 0 };

/**
 *  Theses variables may be changed by interrupt handler code,
 *  so we mark them as `volatile` to ensure compiler optimization
 *  doesn't render them immutable at runtime
 */
volatile bool received_request = false;
volatile uint8_t item_number = 1;

// Central store for notification records.
// Holds one record at a time -- each record is 16 bytes.
volatile struct MvNotification http_notification_center[16];


/**
 *  @brief The application entry point.
 */
int main(void) {
    // Initialize peripherals
    gpio_init();

    // Get the Device ID and build number and log them
    log_device_info();

    // Set up channel notifications
    http_channel_center_setup();

    // Tick counters
    uint64_t kill_tick = 0;
    uint64_t last_send_tick = 0;
    uint64_t last_led_flash_tick = 0;
    uint64_t tick = 0;

    // HTTP channel management
    bool close_channel = false;

    // Remote debug demo variables
    uint32_t store = 42;
    printf("Debug test variable start value: %lu\n", store);

    // Main program loop
    while (1) {
        enum MvStatus status = mvGetMicroseconds(&tick);
        if (status == MV_STATUS_OKAY && tick - last_led_flash_tick > LED_FLASH_PERIOD_US) {
            // Toggle the USER LED's GPIO pin every LED_FLASH_PERIOD_US microseconds
            HAL_GPIO_TogglePin(LED_GPIO_BANK, LED_GPIO_PIN);
            last_led_flash_tick = tick;
        }

        // Send a periodic HTTP request
        if (status == MV_STATUS_OKAY && tick - last_send_tick > REQUEST_SEND_PERIOD_US) {

            /* **********************************************
             *
             * Remote Debug Demo Entry Point
             * Step into this function with GDB's 's' command
             *
             * **********************************************
             */
            debug_function_parent(&store);
            printf("Debug test variable value: %lu\n", store);

            // No channel open? Try and send the temperature
            if (http_handles.channel == 0 && http_open_channel()) {
                bool result = http_send_request();
                if (!result) close_channel = true;
                kill_tick = tick;
            } else {
                server_error("Channel handle not zero");
            }

            last_send_tick = tick;
        }

        // Process a request's response if indicated by the ISR
        if (received_request) {
            http_process_response();
        }

        // Use 'kill_tick' to force-close an open HTTP channel
        // if it's been left open too long
        if (kill_tick > 0 && tick - kill_tick > CHANNEL_KILL_PERIOD_US) {
            close_channel = true;
        }

        // If we've received a response in an interrupt handler,
        // we can close the HTTP channel for the time being
        if (received_request || close_channel) {
            close_channel = false;
            received_request = false;
            kill_tick = 0;
            http_close_channel();
        }
    }
}


/**
 * @brief Initialize the MCU GPIO.
 *
 * Used to flash the Nucleo's USER LED, which is on GPIO Pin PA5.
 */
void gpio_init(void) {
    // Enable GPIO port clock
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // Configure GPIO pin output Level
    HAL_GPIO_WritePin(LED_GPIO_BANK, LED_GPIO_PIN, GPIO_PIN_RESET);

    // Configure GPIO pin : PA5 - Pin under test
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin   = LED_GPIO_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(LED_GPIO_BANK, &GPIO_InitStruct);
}


/**
 *  @brief Sequence-oriented function to demo remote debugging #1.
 */
void debug_function_parent(uint32_t* vptr) {
    uint32_t test_var = *vptr;
    debug_function_child(&test_var);
    *vptr = test_var;
}


/**
 *  @brief Sequence-oriented function to demo remote debugging #2.
 *
 *  @retval Always `true`, to demo return values.
 */
bool debug_function_child(uint32_t* vptr) {
    (*vptr)++;
    return true;
}


/**
 *  @brief Open a new HTTP channel.
 *
 *  @retval `true` if the channel is open, otherwise `false`.
 */
bool http_open_channel(void) {
    // Set up the HTTP channel's multi-use send and receive buffers
    static volatile uint8_t http_rx_buffer[HTTP_RX_BUFFER_SIZE_B] __attribute__((aligned(512)));
    static volatile uint8_t http_tx_buffer[HTTP_TX_BUFFER_SIZE_B] __attribute__((aligned(512)));
    static const char endpoint[] = "";

    // Get the network channel handle.
    // NOTE This is set in `logging.c` which puts the network in place
    //      (ie. so the network handle != 0) well in advance of this being called
    http_handles.network = get_net_handle();
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
            .endpoint            = (uint8_t*)endpoint,
            .endpoint_len        = 0
        }
    };

    // Ask Microvisor to open the channel
    // and confirm that it has accepted the request
    enum MvStatus status = mvOpenChannel(&channel_config, &http_handles.channel);
    if (status == MV_STATUS_OKAY) {
        server_log("HTTP channel handle: %lu", (uint32_t)http_handles.channel);
        return true;
    } else {
        server_error("HTTP channel opening failed. Status: %i", status);
    }

    return false;
}


/**
 *  @brief Close the currently open HTTP channel.
 */
void http_close_channel(void) {
    // If we have a valid channel handle -- ie. it is non-zero --
    // then ask Microvisor to close it and confirm acceptance of
    // the closure request.
    if (http_handles.channel != 0) {
        enum MvStatus status = mvCloseChannel(&http_handles.channel);
        assert((status == MV_STATUS_OKAY || status == MV_STATUS_CHANNELCLOSED) && "[ERROR] Channel closure");
        server_log("HTTP channel closed");
    }

    // Confirm the channel handle has been invalidated by Microvisor
    assert((http_handles.channel == 0) && "[ERROR] Channel handle not zero");
}


/**
 * @brief Configure the channel Notification Center.
 */
void http_channel_center_setup(void) {
    // Clear the notification store
    memset((void *)http_notification_center, 0xFF, sizeof(http_notification_center));

    // Configure a notification center for network-centric notifications
    static struct MvNotificationSetup http_notification_setup = {
        .irq = TIM8_BRK_IRQn,
        .buffer = (struct MvNotification *)http_notification_center,
        .buffer_size = sizeof(http_notification_center)
    };

    // Ask Microvisor to establish the notification center
    // and confirm that it has accepted the request
    enum MvStatus status = mvSetupNotifications(&http_notification_setup, &http_handles.notification);
    assert((status == MV_STATUS_OKAY) && "[ERROR] Could not set up HTTP channel NC");

    // Start the notification IRQ
    NVIC_ClearPendingIRQ(TIM8_BRK_IRQn);
    NVIC_EnableIRQ(TIM8_BRK_IRQn);
    server_log("Notification center handle: %lu", (uint32_t)http_handles.notification);
}


/**
 * @brief Send a stock HTTP request.
 *
 * @returns `true` if the request was accepted by Microvisor, otherwise `false`
 */
bool http_send_request() {
    // Make sure we have a valid channel handle
    if (http_handles.channel != 0) {
        server_log("Sending HTTP request");

        // Set up the request
        const char verb[] = "GET";
        const char body[] = "";
        char uri[46] = "";
        sprintf(uri, "https://jsonplaceholder.typicode.com/todos/%u", item_number);
        struct MvHttpHeader hdrs[] = {};
        struct MvHttpRequest request_config = {
            .method = (uint8_t *)verb,
            .method_len = strlen(verb),
            .url = (uint8_t *)uri,
            .url_len = strlen(uri),
            .num_headers = 0,
            .headers = hdrs,
            .body = (uint8_t *)body,
            .body_len = strlen(body),
            .timeout_ms = 10000
        };

        // FROM 1.1.0
        // Switch the retrieved JSON file
        item_number++;
        if (item_number > 9) item_number = 1;

        // Issue the request -- and check its status
        enum MvStatus status = mvSendHttpRequest(http_handles.channel, &request_config);
        if (status == MV_STATUS_OKAY) {
            server_log("Request sent to Twilio");
            return true;
        }

        // Report send failure
        server_error("Could not issue request. Status: %i", status);
        return false;
    }

    // There's no open channel, so open open one now and
    // try to send again
    http_open_channel();
    return http_send_request();
}


/**
 *  @brief The HTTP channel notification interrupt handler.
 *
 *  This is called by Microvisor -- we need to check for key events
 *  and extract HTTP response data when it is available.
 */
void TIM8_BRK_IRQHandler(void) {
    // Get the event type
    enum MvEventType event_kind = http_notification_center->event_type;

    if (event_kind == MV_EVENTTYPE_CHANNELDATAREADABLE) {
        // Flag we need to access received data and to close the HTTP channel
        // when we're back in the main loop. This lets us exit the ISR quickly.
        // We should not make Microvisor System Calls in the ISR.
        received_request = true;
    }
}


/**
 * @brief Process HTTP response data
 */
void http_process_response(void) {
    // We have received data via the active HTTP channel so establish
    // an `MvHttpResponseData` record to hold response metadata
    static struct MvHttpResponseData resp_data;
    enum MvStatus status = mvReadHttpResponseData(http_handles.channel, &resp_data);
    if (status == MV_STATUS_OKAY) {
        // Check we successfully issued the request (`result` is OK) and
        // the request was successful (status code 200)
        if (resp_data.result == MV_HTTPRESULT_OK) {
            if (resp_data.status_code == 200) {
                server_log("HTTP response header count: %lu", resp_data.num_headers);
                server_log("HTTP response body length: %lu", resp_data.body_length);

                // Set up a buffer that we'll get Microvisor to write
                // the response body into
                uint8_t buffer[resp_data.body_length + 1];
                memset((void *)buffer, 0x00, resp_data.body_length + 1);
                status = mvReadHttpResponseBody(http_handles.channel, 0, buffer, resp_data.body_length);
                if (status == MV_STATUS_OKAY) {
                    // Retrieved the body data successfully so log it
                    printf("%s\n", buffer);
                    //output_headers(resp_data.num_headers);
                } else {
                    server_error("HTTP response body read status %i", status);
                }
            } else {
                server_error("HTTP status code: %lu", resp_data.status_code);
            }
        } else {
            server_error("Request failed. Status: %i", resp_data.result);;
        }
    } else {
        server_error("Response data read failed. Status: %i", status);
    }
}


/**
 * @brief Show basic device info.
 */
void log_device_info(void) {
    uint8_t buffer[35] = { 0 };
    mvGetDeviceId(buffer, 34);
    printf("Device: %s\n   App: %s %s\n Build: %i\n", buffer, APP_NAME, APP_VERSION, BUILD_NUM);
}


/**
 * @brief Issue debug message.
 *
 * @param format_string Message string with optional formatting
 * @param ...           Optional injectable values
 */
void server_log(char* format_string, ...) {
    if (LOG_DEBUG_MESSAGES) {
        va_list args;
        char buffer[512] = "[DEBUG] ";
        va_start(args, format_string);
        vsprintf(&buffer[8], format_string, args);
        va_end(args);
        printf("%s\n", buffer);
    }
}


/**
 * @brief Issue error message.
 *
 * @param format_string Message string with optional formatting
 * @param ...           Optional injectable values
 */
void server_error(char* format_string, ...) {
    va_list args;
    char buffer[512] = "[ERROR] ";
    va_start(args, format_string);
    vsprintf(&buffer[8], format_string, args);
    va_end(args);
    printf("%s\n", buffer);
}
