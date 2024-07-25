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
 * STATIC PROTOTYPES
 */
static void gpio_init(void);
static void process_http_response(void);


/*
 *  GLOBALS
 */
static bool reset_count = false;

/**
 *  Theses variables may be changed by interrupt handler code,
 *  so we mark them as `volatile` to ensure compiler optimization
 *  doesn't render them immutable at runtime
 */
volatile bool received_request = false;
volatile bool channel_was_closed = false;


/**
 *  @brief The application entry point.
 */
int main(void) {

    // Reset of all peripherals, Initializes the Flash interface and the sys tick.
    HAL_Init();

    // Configure the system clock
    system_clock_config();

    // Initialize peripherals
    gpio_init();

    // Get the Device ID and build number and log them
    log_device_info();

    // What happened before?
    show_wake_reason();

    // Set up channel notifications
    http_setup_notification_center();

    // Start the network
    net_open_network();

    // Tick counters
    uint64_t kill_tick = 0;
    uint64_t last_send_tick = 0;
    uint64_t last_led_flash_tick = 0;
    uint64_t tick = 0;
    enum MvStatus result = MV_STATUS_OKAY;

    // HTTP channel management
    bool do_close_channel = false;

    // Remote debug demo variables
    uint32_t store = 42;
    server_log("Debug test variable start value: %lu", store);

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
            server_log("Debug test variable value: %lu", store);

            // No channel open? Try and send the temperature
            if (http_get_handle() == 0 && http_open_channel()) {
                result = http_send_request(reset_count);
                if (reset_count) reset_count = false;
                if (result > 0) do_close_channel = true;
                kill_tick = tick;
            } else {
                server_error("Channel handle not zero");
            }

            last_send_tick = tick;
        }

        // Respond to unexpected channel closure
        if (channel_was_closed) {
            enum MvClosureReason reason = 0;
            if (mvGetChannelClosureReason(http_get_handle(), &reason) == MV_STATUS_OKAY) {
                server_error("Channel closed for reason: %lu", (uint32_t)reason);
            } else {
                server_error("channel closed for unknown reason");
            }

            channel_was_closed = false;
            do_close_channel = true;
        }

        // Use 'kill_tick' to force-close an open HTTP channel
        // if it's been left open too long
        if (kill_tick > 0 && tick - kill_tick > CHANNEL_KILL_PERIOD_US) {
            server_error("HTTP request timed out");
            do_close_channel = true;
        }

        // Process a request's response if indicated by the ISR
        if (received_request) {
            process_http_response();
        }

        // If we've received a response in an interrupt handler,
        // we can close the HTTP channel for the time being
        if (received_request || do_close_channel) {
            do_close_channel = false;
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
static void gpio_init(void) {

    // Enable GPIO port clock
    __HAL_RCC_GPIOA_CLK_ENABLE()

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
 *  @returns Always `true`, to demo return values.
 */
bool debug_function_child(uint32_t* vptr) {

    (*vptr)++;
    return true;
}


/**
 * @brief Process HTTP response data
 */
static void process_http_response(void) {

    // We have received data via the active HTTP channel so establish
    // an `MvHttpResponseData` record to hold response metadata
    static struct MvHttpResponseData resp_data;
    enum MvStatus status = mvReadHttpResponseData(http_get_handle(), &resp_data);
    if (status == MV_STATUS_OKAY) {
        // Check we successfully issued the request (`result` is OK) and
        // the request was successful (status code 200)
        if (resp_data.result == MV_HTTPRESULT_OK) {
            if (resp_data.status_code == 200) {
                server_log("HTTP response received -- body length is %lu bytes, there are %lu headers", resp_data.body_length, resp_data.num_headers);

                // Set up a buffer that we'll get Microvisor to write
                // the response body into
                uint8_t buffer[resp_data.body_length + 1];
                memset((void *)buffer, 0x00, resp_data.body_length + 1);
                status = mvReadHttpResponseBody(http_get_handle(), 0, buffer, resp_data.body_length);
                if (status == MV_STATUS_OKAY) {
                    // Retrieved the body data successfully so log it
                    server_log("Message JSON:\n%s", buffer);
                } else {
                    server_error("HTTP response body read status %i", status);
                }
            } else if (resp_data.status_code == 404) {
                // Reached the end of available items, so reset the counter
                reset_count = true;
                server_log("Resetting ping count");
            } else {
                server_error("HTTP status code: %lu", resp_data.status_code);
            }
        } else {
            server_error("Request failed. Status: %i", resp_data.result);
        }
    } else {
        server_error("Response data read failed. Status: %i", status);
    }
}


