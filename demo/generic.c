/**
 *
 * Microvisor C Demos
 *
 * Copyright © 2024, KORE Wireless
 * Licence: MIT
 *
 */
#include "main.h"
#include "app_version.h"


/**
 * @brief Get the MV clock value.
 *
 * @returns The clock value.
 */
uint32_t SECURE_SystemCoreClockUpdate(void) {

    uint32_t clock = 0;
    mvGetHClk(&clock);
    return clock;
}


/**
 * @brief System clock configuration.
 */
void system_clock_config(void) {

    SystemCoreClockUpdate();
    HAL_InitTick(TICK_INT_PRIORITY);
}


/**
 * @brief Show basic device info.
 */
void show_wake_reason(void) {

    static char* reasons[] = {
        "Cold boot or wake-up from shutdown mode",
        "Microvisor restart requested via server",
        "Application restart requested via server",
        "Application restarted by debugger",
        "Microvisor kernel crash",
        "Microvisor watchdog failure",
        "Microvisor out of memory error",
        "Unspecified Microvisor error",
        "Application crash",
        "Application updated",
        "Microvisor updated",
        "Device option bytes updated",
        "Device woken from deep sleep due to check-in period expiration",
        "Device woken from deep sleep by application",
        "Device woken from deep sleep due to cellular modem interrupt",
        "Device woken from deep sleep due to application RTC wakeup",
        "Device woken from deep sleep: reason unclear"
    };

    enum MvWakeReason reason;
    if (mvGetWakeReason(&reason) == MV_STATUS_OKAY && reason < 17) {
        server_log("Wake reason: %s", reasons[reason]);
        return;
    }

    server_log("Wake reason: Unknown");
}


/**
 * @brief Show basic device info.
 */
void log_device_info(void) {

    uint8_t buffer[35] = { 0 };
    mvGetDeviceId(buffer, 34);
    server_log("Device: %s", buffer);
    server_log("   App: %s %s-%u", APP_NAME, APP_VERSION, BUILD_NUM);
}


/**
 * @brief Enable or disable the Microvisor system LED.
 *        NOTE If disabled, connection state can not be determined visually.
 *
 * @param do_enable: `true` to enable the system LED, `false` to disable.
 */
void control_system_led(bool do_enable) {

    enum MvStatus status = mvSystemLedEnable(do_enable ? 1 : 0);
    assert(status == MV_STATUS_OKAY);
}