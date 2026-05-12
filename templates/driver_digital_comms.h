// driver_digital_comms.h — Template for digital communication protocol drivers
// =============================================================================
// Use this template for devices that use a custom digital protocol over a
// single GPIO pin (bit-banging), such as:
//   - DHT11 / DHT22 / AM2302 (temperature + humidity)
//   - DS18B20 (1-Wire)
//   - Custom sensor protocols with start/stop bits and data frames.
//
// PATTERN: Constructor configures the pin as open-drain (bi-directional).
//          The driver manually toggles the pin to send start signals,
//          then reads timed pulses to decode bits.
//          Protocol-specific timing constants are defined as macros.
//          getReading() handles the full read cycle and returns a struct.
//
// ESP-IDF v5.4 helpers used:
//   - ets_delay_us()  for microsecond delays (from "rom/ets_sys.h")
//   - esp_timer_get_time() for high-resolution timing (microseconds)
//   - gpio_set_direction() to switch between OUTPUT and INPUT
//
// INSTRUCTIONS:
//   1. Copy and rename (e.g. CDHT22.h).
//   2. Replace __DEVICE_NAME__ with your class name.
//   3. Set protocol timing constants (START_LOW_US, BIT_START_US, etc.).
//   4. Implement the data decoding in getReading().
//   5. Customize the reading struct with all sensor values.
// =============================================================================

#ifndef _DRIVER_DIGITAL_COMMS_H_
#define _DRIVER_DIGITAL_COMMS_H_

#include "driver/gpio.h"       // gpio_num_t
#include <stdint.h>

// --- PIN DEFINITION ----------------------------------------------------------
// TODO: Set your GPIO pin number
#define __DEVICE_NAME___GPIO  GPIO_NUM_15

// --- PROTOCOL TIMING (in microseconds) ---------------------------------------
// TODO: Adjust these based on the device's datasheet.
//       The defaults below correspond to DHT22/AM2302.
#define __DEVICE_NAME___START_LOW_US     10000   // Host pulls low for 10 ms
#define __DEVICE_NAME___START_HIGH_US    30      // Host releases, waits 30 µs
#define __DEVICE_NAME___RESPONSE_LOW_US  80      // Device pulls low for 80 µs
#define __DEVICE_NAME___RESPONSE_HIGH_US 80      // Device pulls high for 80 µs
#define __DEVICE_NAME___BIT_START_US     50      // Each bit starts with 50 µs low
#define __DEVICE_NAME___BIT_0_HIGH_US    28      // ~28 µs high → bit 0
#define __DEVICE_NAME___BIT_1_HIGH_US    70      // ~70 µs high → bit 1
#define __DEVICE_NAME___BIT_THRESHOLD_US 45      // Threshold: >45 µs = bit 1
#define __DEVICE_NAME___TIMEOUT_US       150     // Timeout waiting for pin level change
#define __DEVICE_NAME___READ_DELAY_US    5000    // Min interval between reads (5 ms)
#define __DEVICE_NAME___SAMPLE_INTERVAL_US 5     // Sampling interval for wave capture

// --- DATA FRAME --------------------------------------------------------------
// TODO: Adjust frame size, byte layout, checksum formula.
#define __DEVICE_NAME___NUM_BYTES    5           // 5 bytes = 40 bits
#define __DEVICE_NAME___NUM_BITS     (__DEVICE_NAME___NUM_BYTES * 8)

// --- READING STRUCT ----------------------------------------------------------
// TODO: Customize with the values your device returns.
typedef struct {
    // --- Raw frame bytes ---
    uint8_t data[__DEVICE_NAME___NUM_BYTES]; // Raw data bytes

    // --- Parsed sensor values ---
    // Example for DHT22:
    // float   humidity;        // Relative humidity (%)
    // float   temperature;     // Temperature (°C)
    // uint8_t checksum;        // Checksum byte received

    bool    valid;              // True if CRC/checksum verified
} __device_name___reading_t;


// --- CLASS DECLARATION -------------------------------------------------------
class __DeviceName__ {

public:
    // -------------------------------------------------------------------------
    // Constructor — configures the GPIO pin as bi-directional (open-drain).
    // @param port  GPIO pin number
    // -------------------------------------------------------------------------
    __DeviceName__(int port);

    // -------------------------------------------------------------------------
    // getReading() — Perform a full read cycle (start signal → read frame).
    //                Returns a reading struct with parsed values.
    // -------------------------------------------------------------------------
    __device_name___reading_t getReading();

    // -------------------------------------------------------------------------
    // printWave() — Debug helper: capture and log the full waveform.
    //               Useful for verifying timing during development.
    // -------------------------------------------------------------------------
    void printWave();

private:
    gpio_num_t   m_pinNumber;       // GPIO pin
    const char*  LogName;           // Tag for ESP_LOGx macros
    int64_t      m_lastReadUs;      // Timestamp of last read for rate limiting

    // --- Low-level protocol helpers ------------------------------------------
    // Wait until the pin reaches the given level, with timeout.
    // Returns the elapsed time in µs (or -1 on timeout).
    int64_t waitForLevel(int level, int64_t timeoutUs = __DEVICE_NAME___TIMEOUT_US);

    // Read a single bit using the protocol's pulse-width encoding.
    //
    // Synchronisation:
    //   1. Wait for FALLING edge  → align with start of bit's LOW period.
    //      (Returns immediately if line is already LOW from previous bit.)
    //   2. Wait for RISING  edge  → LOW period ended, HIGH encoding begins.
    //   3. Measure HIGH duration   → compare against BIT_THRESHOLD_US.
    //
    // This two-step wait makes readBit() robust against misinterpreting
    // a lingering HIGH signal (e.g. from a device response phase) as
    // bit-1 encoding.
    //
    // Returns 0 or 1, or -1 on timeout.
    int readBit();

    // Capture the raw waveform into a buffer (for debugging).
    // @param buffer  Output buffer
    // @param len     Buffer length (number of samples)
    // @param intervalUs  Sampling interval in µs
    void captureWave(int* buffer, int len, int intervalUs);
};

#endif // _DRIVER_DIGITAL_COMMS_H_
