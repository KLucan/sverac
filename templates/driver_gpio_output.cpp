// driver_gpio_output.cpp — Template for digital output device drivers
// =============================================================================
// Fill in the stubs labelled "TODO" with your device-specific logic.
// See driver_gpio_output.h for the full instructions.
// =============================================================================

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"          // esp_timer_get_time()
#include "sdkconfig.h"
#include "__DeviceName__.h"      // TODO: rename to your header

// --- CONSTRUCTOR -------------------------------------------------------------
__DeviceName__::__DeviceName__(int port) {
    m_pinNumber = (gpio_num_t)port;
    m_state     = __DEVICE_NAME___OFF;   // TODO: set default state
    m_lastBlink = 0;
    m_lastBlinkState = 0;
    LogName     = "__DeviceName__";      // TODO: set your log tag

    ESP_LOGI(LogName, "Configure port [%d] to OUTPUT", port);

    // Reset pin to default state
    gpio_reset_pin(m_pinNumber);

    // Set pin direction to OUTPUT
    gpio_set_direction(m_pinNumber, GPIO_MODE_OUTPUT);

    // Initial level: LOW (0)
    gpio_set_level(m_pinNumber, 0);

    // TODO: Add any device-specific initialisation here
    //       (e.g. set drive strength: gpio_set_drive_capability(...))
}

// --- TICK --------------------------------------------------------------------
void __DeviceName__::tick() {
    int64_t now = esp_timer_get_time();

    switch (m_state) {

        case __DEVICE_NAME___OFF:
            gpio_set_level(m_pinNumber, 0);
            break;

        case __DEVICE_NAME___ON:
            gpio_set_level(m_pinNumber, 1);
            break;

        case __DEVICE_NAME___BLINK:
            if ((now - m_lastBlink) > __DEVICE_NAME___BLINK_NORMAL) {
                m_lastBlink = now;
                m_lastBlinkState = (m_lastBlinkState == 0) ? 1 : 0;
                ESP_LOGI(LogName, "BLINK — STATE [%d]", m_lastBlinkState);
                gpio_set_level(m_pinNumber, m_lastBlinkState);
            }
            break;

        case __DEVICE_NAME___SLOW_BLINK:
            if ((now - m_lastBlink) > __DEVICE_NAME___BLINK_SLOW) {
                m_lastBlink = now;
                m_lastBlinkState = (m_lastBlinkState == 0) ? 1 : 0;
                ESP_LOGI(LogName, "SLOW BLINK — STATE [%d]", m_lastBlinkState);
                gpio_set_level(m_pinNumber, m_lastBlinkState);
            }
            break;

        case __DEVICE_NAME___FAST_BLINK:
            if ((now - m_lastBlink) > __DEVICE_NAME___BLINK_FAST) {
                m_lastBlink = now;
                m_lastBlinkState = (m_lastBlinkState == 0) ? 1 : 0;
                ESP_LOGI(LogName, "FAST BLINK — STATE [%d]", m_lastBlinkState);
                gpio_set_level(m_pinNumber, m_lastBlinkState);
            }
            break;

        // TODO: Add cases for your custom states (PULSE, PWM, etc.)
        // case __DEVICE_NAME___PULSE:
        //     ... single-pulse logic ...
        //     break;

        default:
            break;
    }
}

// --- SET STATE ---------------------------------------------------------------
void __DeviceName__::setState(__DeviceName__State x) {
    ESP_LOGI(LogName, "Set state → %d", (int)x);
    m_state = x;

    // TODO: If your device needs immediate action on state change, add it here.
    //       Example: if switching to ON, set the pin high immediately.
}

// --- GET STATE ---------------------------------------------------------------
__DeviceName__State __DeviceName__::getState() {
    return m_state;
}

// --- SET LEVEL (direct GPIO control) -----------------------------------------
void __DeviceName__::setLevel(int level) {
    ESP_LOGI(LogName, "Set level → %d", level);
    gpio_set_level(m_pinNumber, level);
}

// --- GET LEVEL ---------------------------------------------------------------
int __DeviceName__::getLevel() {
    return gpio_get_level(m_pinNumber);
}

// --- GET READING (if device provides feedback) -------------------------------
__device_name___reading_t __DeviceName__::getReading() {
    __device_name___reading_t reading = {};
    reading.level = gpio_get_level(m_pinNumber);
    reading.state = (int)m_state;
    reading.valid = true;   // TODO: validate if needed
    return reading;
}
