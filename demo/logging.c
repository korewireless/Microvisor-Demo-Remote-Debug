/**
 *
 * Microvisor Remote Debugging Demo
 *
 * Copyright © 2023, KORE Wireless
 * Licence: MIT
 *
 */
#include "main.h"


/*
 * STATIC PROTOTYPES
 */
static void log_start(void);
static void log_service_setup(void);
static void post_log(bool is_err, char* format_string, va_list args);


/*
 * GLOBALS
 */
// Entities for Microvisor application logging
static uint8_t log_buffer[LOG_BUFFER_SIZE_B] __attribute__((aligned(512))) = {0};
static uint32_t log_state = USER_HANDLE_LOGGING_OFF;

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
    
    if (log_state != USER_HANDLE_LOGGING_STARTED) {
        // Initiate the Microvisor logging service
        log_service_setup();

#if ENABLE_UART_DEBUGGING == true
        // Establish UART logging
        uart_available = log_uart_init();
#endif
    }
}


/**
 * @brief Initiate Microvisor application logging.
 */
static void log_service_setup(void) {
    
    // Initialize logging with the standard system call
    enum MvStatus status = mvServerLoggingInit(log_buffer, LOG_BUFFER_SIZE_B);

    // Set a mock handle as a proxy for a 'logging enabled' flag
    if (status == MV_STATUS_OKAY) log_state = USER_HANDLE_LOGGING_STARTED;
    do_assert(status == MV_STATUS_OKAY, "Could not start logging");
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
    
    log_start();
    static char buffer[LOG_MESSAGE_MAX_LEN_B] = {0};

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
