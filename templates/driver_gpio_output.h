// driver_gpio_output.h — Template for digital output device drivers
// =============================================================================
// Use this template for devices that need simple GPIO output control:
//   - LEDs, relays, buzzers, solenoids, etc.
//
// PATTERN: Constructor sets up the GPIO pin. Call tick() periodically to update
//          timed behaviour (blinking, pulses). Call setState() / setLevel()
//          to change output on demand.
//
// INSTRUCTIONS:
//   1. Copy this file and rename (e.g. CMyOutput.h).
//   2. Replace all occurrences of __DEVICE_NAME__ with your class name.
//   3. Add device-specific state enum values if needed.
//   4. Adjust timing constants (in microseconds, 1 000 000 = 1 second).
//   5. Customize the reading struct if your device returns data.
// =============================================================================

#ifndef _DRIVER_GPIO_OUTPUT_H_
#define _DRIVER_GPIO_OUTPUT_H_

#include "driver/gpio.h"       // gpio_num_t, gpio_set_level, etc.
#include <stdint.h>            // int64_t

// --- PIN DEFINITION ----------------------------------------------------------
// TODO: Set your GPIO pin number here (or pass via constructor)
#define __DEVICE_NAME___GPIO  GPIO_NUM_2

// --- TIMING CONSTANTS (in microseconds) --------------------------------------
// TODO: Adjust these to match your device's required timing
#define __DEVICE_NAME___BLINK_FAST    250000L   // 250 ms
#define __DEVICE_NAME___BLINK_NORMAL  1000000L  // 1 s
#define __DEVICE_NAME___BLINK_SLOW    2000000L  // 2 s

// --- OUTPUT STATE ENUM -------------------------------------------------------
// TODO: Add/remove states as needed for your device
enum __DeviceName__State {
    __DEVICE_NAME___OFF = 0,
    __DEVICE_NAME___ON,
    __DEVICE_NAME___BLINK,
    __DEVICE_NAME___FAST_BLINK,
    __DEVICE_NAME___SLOW_BLINK,
    // __DEVICE_NAME___PULSE,    // single pulse
    // __DEVICE_NAME___PWM,      // if using LEDC/PWM
};

// --- READING STRUCT (if device provides feedback) ----------------------------
// TODO: Define a struct if your output device also returns readings.
//       Remove if not needed.
typedef struct {
    int     level;      // Current output level (0 or 1)
    int     state;      // Current state from the enum
    bool    valid;      // Whether the reading is valid
} __device_name___reading_t;


// --- CLASS DECLARATION -------------------------------------------------------
class __DeviceName__ {

public:
    // -------------------------------------------------------------------------
    // Constructor — configures the GPIO pin
    // @param port  GPIO pin number (use GPIO_NUM_X macros)
    // -------------------------------------------------------------------------
    __DeviceName__(int port);

    // -------------------------------------------------------------------------
    // tick() — Must be called periodically (e.g. from a task every 20–100 ms).
    //          Handles timed state transitions (blinking etc.).
    // -------------------------------------------------------------------------
    void tick();

    // -------------------------------------------------------------------------
    // setState() — Change the logical output state
    // @param x  Desired state from __DeviceName__State enum
    // -------------------------------------------------------------------------
    void setState(__DeviceName__State x);

    // -------------------------------------------------------------------------
    // getState() — Return the current logical state
    // -------------------------------------------------------------------------
    __DeviceName__State getState();

    // -------------------------------------------------------------------------
    // setLevel() — Directly set GPIO level (0 or 1) — bypasses state machine
    // -------------------------------------------------------------------------
    void setLevel(int level);

    // -------------------------------------------------------------------------
    // getLevel() — Return current GPIO level
    // -------------------------------------------------------------------------
    int getLevel();

    // -------------------------------------------------------------------------
    // getReading() — Return a reading struct (if device provides feedback)
    //                TODO: Remove if not needed.
    // -------------------------------------------------------------------------
    __device_name___reading_t getReading();

private:
    gpio_num_t          m_pinNumber;        // GPIO pin
    __DeviceName__State m_state;            // Current logical state
    const char*         LogName;            // Tag for ESP_LOGx macros
    int64_t             m_lastBlink;        // Timestamp of last blink toggle (us)
    int                 m_lastBlinkState;   // Current blink level (0 or 1)
};

#endif // _DRIVER_GPIO_OUTPUT_H_
