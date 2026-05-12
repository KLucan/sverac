// driver_gpio_input.h — Template for digital input device drivers
// =============================================================================
// Use this template for devices that read digital signals:
//   - Buttons, switches, reed sensors, hall-effect digital outputs, etc.
//
// PATTERN: Constructor configures the pin as input (with optional pull-up/down).
//          tick() is called periodically and runs a state machine to debounce,
//          detect edges, long-press, double-click etc.
//          User attaches callback functions that fire on detected events.
//
// INSTRUCTIONS:
//   1. Copy and rename (e.g. CButton.h).
//   2. Replace __DEVICE_NAME__ with your class name.
//   3. Adjust timing constants for debounce, long-press, double-click windows.
//   4. Add/remove event types in the state machine.
//   5. Customize the reading struct.
// =============================================================================

#ifndef _DRIVER_GPIO_INPUT_H_
#define _DRIVER_GPIO_INPUT_H_

#include "driver/gpio.h"       // gpio_num_t
#include <stdint.h>            // int64_t

// --- PIN DEFINITION ----------------------------------------------------------
// TODO: Set your GPIO pin number here (or pass via constructor)
#define __DEVICE_NAME___GPIO  GPIO_NUM_0

// --- TIMING CONSTANTS (in microseconds) --------------------------------------
// TODO: Adjust these to match your device's characteristics
#define __DEVICE_NAME___DEBOUNCE_US       50000L    //  50 ms debounce
#define __DEVICE_NAME___DOUBLECLICK_US    300000L   // 300 ms double-click window
#define __DEVICE_NAME___LONGPRESS_US      1000000L  //   1 s long-press threshold
#define __DEVICE_NAME___REPEAT_US         200000L   // 200 ms repeat while held

// --- INPUT STATE ENUM --------------------------------------------------------
// TODO: Extend or trim states as needed for your device
//
// DESIGN NOTE — Why double-click targets LONG_PRESSED instead of IDLE:
//   When a double-click is detected in JUST_RELEASED, the input is still
//   physically active (the second press is being held).  Going to IDLE would
//   start a spurious new press cycle that fires onClick() on release.
//   LONG_PRESSED is reused here as a generic "wait for release" sink — it
//   silently waits for the input to de-assert, then returns to IDLE.
//   If you rename or replace LONG_PRESSED, preserve this behaviour.
enum __DeviceName__InputState {
    __DEVICE_NAME___IDLE = 0,           // Waiting for activity
    __DEVICE_NAME___DEBOUNCE,           // Debouncing after initial edge
    __DEVICE_NAME___PRESSED,            // Confirmed press — waiting for release or long-press
    __DEVICE_NAME___JUST_RELEASED,      // Just released — check for double-click
    __DEVICE_NAME___LONG_PRESSED,       // Held longer than LONGPRESS_US (also used as
                                        //   post-double-click "wait for release" sink)
    // __DEVICE_NAME___REPEAT,          // Auto-repeat while held
};

// --- CALLBACK FUNCTION POINTER TYPE ------------------------------------------
// TODO: Adjust signature if your callbacks need parameters (e.g. pin number).
//       Keep extern "C" for C compatibility if mixing C/C++ code.
extern "C" {
    typedef void (*InputEventHandler)(void);
}
// Usage example:  void myOnClick() { /* ... */ }

// --- READING STRUCT (if your input device returns data) ----------------------
typedef struct {
    int     level;          // Current pin level (0 or 1)
    int     state;          // Current state-machine state
    int     eventCount;     // Incremented on each detected event
    bool    valid;          // Whether the reading is valid
} __device_name___reading_t;


// --- CLASS DECLARATION -------------------------------------------------------
class __DeviceName__ {

public:
    // -------------------------------------------------------------------------
    // Constructor — configures GPIO as input with pull-up/down
    // @param port             GPIO pin number
    // @param activeLevel      0 = active-low, 1 = active-high (default: 0)
    // @param pullMode         GPIO_PULLUP_ONLY, GPIO_PULLDOWN_ONLY, GPIO_FLOATING
    // -------------------------------------------------------------------------
    __DeviceName__(int port, int activeLevel = 0,
                   gpio_pull_mode_t pullMode = GPIO_PULLUP_ONLY);

    // -------------------------------------------------------------------------
    // tick() — Must be called periodically (e.g. every 10–50 ms).
    //          Runs the debounce/state machine and fires callbacks.
    // -------------------------------------------------------------------------
    void tick();

    // --- CALLBACK ATTACHMENT METHODS -----------------------------------------
    // TODO: Add/remove as needed for your device events.
    void attachOnPress(InputEventHandler method)    { onPress    = method; }
    void attachOnRelease(InputEventHandler method)  { onRelease  = method; }
    void attachOnClick(InputEventHandler method)    { onClick    = method; }
    void attachOnDoubleClick(InputEventHandler method) { onDoubleClick = method; }
    void attachOnLongPress(InputEventHandler method){ onLongPress = method; }
    // void attachOnRepeat(InputEventHandler method)  { onRepeat   = method; }

    // -------------------------------------------------------------------------
    // getLevel() — Return the raw GPIO level (0 or 1)
    // -------------------------------------------------------------------------
    int getLevel();

    // -------------------------------------------------------------------------
    // isActive() — Return true if the input is in "active" state
    //              (accounts for active-low vs active-high)
    // -------------------------------------------------------------------------
    bool isActive();

    // -------------------------------------------------------------------------
    // getReading() — Return a snapshot of the input state
    // -------------------------------------------------------------------------
    __device_name___reading_t getReading();

private:
    gpio_num_t          m_pinNumber;        // GPIO pin
    int                 m_activeLevel;       // 0 = active-low, 1 = active-high
    __DeviceName__InputState m_state;        // Current state-machine state
    const char*         LogName;            // Tag for ESP_LOGx macros
    int64_t             m_lastPulse;        // Timestamp of last edge (us)

    // --- Callback pointers ---------------------------------------------------
    InputEventHandler   onPress         = nullptr;
    InputEventHandler   onRelease       = nullptr;
    InputEventHandler   onClick         = nullptr;
    InputEventHandler   onDoubleClick   = nullptr;
    InputEventHandler   onLongPress     = nullptr;
    // InputEventHandler   onRepeat        = nullptr;

    // --- Helper methods ------------------------------------------------------
    bool rawActive();   // true if raw pin level matches active level
};

#endif // _DRIVER_GPIO_INPUT_H_
