/**
 *
 * Microvisor HTTP Communications Demo
 * Version 2.0.8
 * Copyright © 2023, Twilio
 * Licence: Apache 2.0
 *
 */
#include "main.h"


UART_HandleTypeDef log_uart;


/**
 * @brief Configure STM32U585 UART2.
 */
bool log_uart_init(void) {
    
    log_uart.Instance           = USART2;
    log_uart.Init.BaudRate      = 115200;              // Set your preferred speed
    log_uart.Init.WordLength    = UART_WORDLENGTH_8B;  // 8
    log_uart.Init.StopBits      = UART_STOPBITS_1;     // N
    log_uart.Init.Parity        = UART_PARITY_NONE;    // 1
    log_uart.Init.Mode          = UART_MODE_TX;        // TX only mode
    log_uart.Init.HwFlowCtl     = UART_HWCONTROL_NONE; // No CTS/RTS

    // Initialize the UART
    if (HAL_UART_Init(&log_uart) != HAL_OK) {
      // Log error
      server_log("Could not enable logging UART");
      return false;
    }

    server_log("UART logging enabled");
    return true;
}

/**
 * @brief HAL-called function to configure UART.
 *
 * @param uart: A HAL UART_HandleTypeDef pointer to the UART instance.
 */
void HAL_UART_MspInit(UART_HandleTypeDef *uart) {
    
    // This SDK-named function is called by HAL_UART_Init()

    // Configure U5 peripheral clock
    RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
    PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;

    // Initialize U5 peripheral clock
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        // Log error
        server_log("Could not enable logging UART clock");
        return;
    }

    // Enable the UART GPIO interface clock
    __HAL_RCC_GPIOD_CLK_ENABLE();

    // Configure the GPIO pins for UART
    // Pin PD5 - TX
    GPIO_InitTypeDef gpioConfig = { 0 };
    gpioConfig.Pin       = GPIO_PIN_5;              // TX pin
    gpioConfig.Mode      = GPIO_MODE_AF_PP;         // Pin's alt function with pull...
    gpioConfig.Pull      = GPIO_NOPULL;             // ...but don't apply a pull
    gpioConfig.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpioConfig.Alternate = GPIO_AF7_USART2;         // Select the alt function

    // Initialize the pins with the setup data
    HAL_GPIO_Init(GPIOD, &gpioConfig);


    // Enable the UART clock
    __HAL_RCC_USART2_CLK_ENABLE();
}


/**
 * @brief Output a UART-friendly log string, ie. one with
 *        RETURN+NEWLINE in place of NEWLINE.
 *
 * @param buffer: Source string.
 * @param length: String character count.
 */
void log_uart_output(char* buffer) {
    
    char uart_buffer[UART_LOG_TIMESTAMP_MAX_LEN_B + LOG_MESSAGE_MAX_LEN_B + 3] = {0};
    
    uint64_t usec = 0;
    time_t sec = 0;
    time_t msec = 0;
    
    enum MvStatus status = mvGetWallTime(&usec);
    if (status == MV_STATUS_OKAY) {
        // Get the second and millisecond times
        sec = (time_t)usec / 1000000;
        msec = (time_t)usec / 1000;
    }
    
    // Write time string as "2022-05-10 13:30:58.XXX "
    strftime(uart_buffer, 64, "%F %T.XXX ", gmtime(&sec));
    // Insert the millisecond time over the XXX
    sprintf(&uart_buffer[20], "%03u ", (unsigned)(msec % 1000));
    size_t length = strlen(uart_buffer);
    
    // Write the timestamp to the message
    sprintf(&uart_buffer[length], "%s\n", buffer);
    
    // Send uart_buffer to the UART
    const char nls[2] = "\r\n";
    char *buf_ptr = uart_buffer;
    while(*buf_ptr != 0) {
        if (*buf_ptr == '\n') {
            HAL_UART_Transmit(&log_uart, (uint8_t*)nls, 2, 100);
            buf_ptr++;
        } else {
            HAL_UART_Transmit(&log_uart, (uint8_t*)buf_ptr++, 1, 100);
        }
    }
}
