// driver_gpio_input.cpp — Template for digital input device drivers
// =============================================================================
// Fill in the stubs labelled "TODO" with your device-specific logic.
// See driver_gpio_input.h for the full instructions.
// =============================================================================

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"          // esp_timer_get_time()
#include "__DeviceName__.h"      // TODO: rename to your header

// --- CONSTRUCTOR -------------------------------------------------------------
__DeviceName__::__DeviceName__(int port, int activeLevel, gpio_pull_mode_t pullMode) {
    m_pinNumber    = (gpio_num_t)port;
    m_activeLevel  = activeLevel;
    m_state        = __DEVICE_NAME___IDLE;
    m_lastPulse    = 0;
    LogName        = "__DeviceName__";   // TODO: set your log tag

    ESP_LOGI(LogName, "Configure port [%d] to INPUT (active=%s, pull=%d)",
             port, activeLevel ? "HIGH" : "LOW", (int)pullMode);

    // Reset pin to default state
    gpio_reset_pin(m_pinNumber);

    // Set pin direction to INPUT
    gpio_set_direction(m_pinNumber, GPIO_MODE_INPUT);

    // Configure pull-up or pull-down
    gpio_set_pull_mode(m_pinNumber, pullMode);

    // TODO: Add any device-specific initialisation here
    //       (e.g., enable interrupt if using ISR: see main_interrupt.cpp)
}

// --- RAW ACTIVE HELPER -------------------------------------------------------
bool __DeviceName__::rawActive() {
    // Returns true if the raw GPIO level equals the active level
    return (gpio_get_level(m_pinNumber) == m_activeLevel);
}

// --- TICK (state machine) ----------------------------------------------------
void __DeviceName__::tick() {
    int64_t now = esp_timer_get_time();

    switch (m_state) {

        // --- IDLE: waiting for initial edge ----------------------------------
        case __DEVICE_NAME___IDLE: {
            // Wait for debounce window to expire (avoid re-triggering too fast)
            if ((now - m_lastPulse) < __DEVICE_NAME___DEBOUNCE_US) break;

            if (rawActive()) {
                m_lastPulse = now;
                ESP_LOGD(LogName, "EDGE DETECTED → entering PRESSED");
                m_state = __DEVICE_NAME___PRESSED;

                // Fire onPress callback if attached
                if (onPress != nullptr) onPress();
            }
            break;
        }

        // --- PRESSED: waiting for release or long-press ----------------------
        case __DEVICE_NAME___PRESSED: {
            // Check for long-press
            if (rawActive() &&
                (now - m_lastPulse) > __DEVICE_NAME___LONGPRESS_US) {
                ESP_LOGI(LogName, "LONG PRESS DETECTED");
                m_lastPulse = now;
                m_state = __DEVICE_NAME___LONG_PRESSED;

                // Fire long-press callback
                if (onLongPress != nullptr) onLongPress();
            }
            // Check for release
            else if (!rawActive()) {
                ESP_LOGD(LogName, "RELEASED → entering JUST_RELEASED");
                m_lastPulse = now;
                m_state = __DEVICE_NAME___JUST_RELEASED;

                // Fire onRelease callback
                if (onRelease != nullptr) onRelease();
            }
            break;
        }

        // --- JUST_RELEASED: check for double-click or finalise single-click --
        case __DEVICE_NAME___JUST_RELEASED: {
            // If another press comes within the double-click window:
            if (rawActive() &&
                (now - m_lastPulse) < __DEVICE_NAME___DOUBLECLICK_US) {
                ESP_LOGI(LogName, "DOUBLE CLICK DETECTED");
                m_lastPulse = now;
                // IMPORTANT: Go to LONG_PRESSED, NOT IDLE.
                // At this moment the input is still physically active
                // (the second press of the double-click is being held).
                // LONG_PRESSED silently waits for release and then returns
                // to IDLE.  If we went directly to IDLE, the still-active
                // input would immediately start a spurious new press cycle
                // that fires onClick() when released.
                m_state = __DEVICE_NAME___LONG_PRESSED;

                // Fire double-click callback
                if (onDoubleClick != nullptr) onDoubleClick();
            }
            // Double-click window expired → finalise as single click
            else if ((now - m_lastPulse) >= __DEVICE_NAME___DOUBLECLICK_US) {
                ESP_LOGI(LogName, "SINGLE CLICK DETECTED");
                m_lastPulse = now;
                m_state = __DEVICE_NAME___IDLE;

                // Fire single-click callback
                if (onClick != nullptr) onClick();
            }
            break;
        }

        // --- LONG_PRESSED: waiting for release -------------------------------
        case __DEVICE_NAME___LONG_PRESSED: {
            // Stay in this state while still pressed.
            // TODO: Optionally fire onRepeat callback here for auto-repeat.
            if (!rawActive()) {
                ESP_LOGD(LogName, "LONG PRESS RELEASED → IDLE");
                m_state = __DEVICE_NAME___IDLE;
            }
            break;
        }

        default:
            break;
    }
}

// --- GET LEVEL ---------------------------------------------------------------
int __DeviceName__::getLevel() {
    return gpio_get_level(m_pinNumber);
}

// --- IS ACTIVE ---------------------------------------------------------------
bool __DeviceName__::isActive() {
    return rawActive();
}

// --- GET READING -------------------------------------------------------------
__device_name___reading_t __DeviceName__::getReading() {
    __device_name___reading_t reading = {};
    reading.level  = gpio_get_level(m_pinNumber);
    reading.state  = (int)m_state;
    reading.valid  = true;   // TODO: validate if needed
    // reading.eventCount = ...   // TODO: increment on events if desired
    return reading;
}
