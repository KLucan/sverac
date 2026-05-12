// segment_bitbang.cpp — Reusable bit-banging helpers for digital protocols
// =============================================================================
// Implementation of the functions declared in segment_bitbang.h.
// Include this .cpp in your CMakeLists.txt SRCS when needed, or copy the
// functions directly into your driver's .cpp file.
// =============================================================================

#include "segment_bitbang.h"
#include "driver/gpio.h"
#include "esp_timer.h"          // esp_timer_get_time()
#include "rom/ets_sys.h"        // ets_delay_us()
#include "esp_log.h"

static const char* BITBANG_TAG = "BITBANG";

// --- waitForLevel ------------------------------------------------------------
int64_t bitbang_waitForLevel(gpio_num_t pin, int level, int64_t timeoutUs) {
    if (timeoutUs <= 0) {
        timeoutUs = BITBANG_DEFAULT_TIMEOUT_US;
    }

    int64_t start = esp_timer_get_time();
    while (gpio_get_level(pin) != level) {
        ets_delay_us(1);   // ~1 µs polling resolution
        int64_t elapsed = esp_timer_get_time() - start;
        if (elapsed > timeoutUs) {
            return -1;   // timeout
        }
    }
    return esp_timer_get_time() - start;   // elapsed µs
}

// --- pulseOut ----------------------------------------------------------------
void bitbang_pulseOut(gpio_num_t pin, int level, int64_t durationUs) {
    gpio_set_level(pin, level);
    ets_delay_us((uint32_t)durationUs);
}

// --- readTimedBit ------------------------------------------------------------
// Reads a single pulse-width-encoded bit.
//
// Protocol assumption:
//   Each bit starts with a LOW period, then a HIGH pulse whose duration
//   determines the bit value:
//     - short HIGH  (< threshold) → bit 0
//     - long HIGH   (> threshold) → bit 1
//
// Synchronisation strategy:
//   1. Wait for FALLING edge  → align with start of bit's LOW period.
//      (Returns immediately if line is already LOW from previous bit.)
//   2. Wait for RISING  edge  → LOW period ended, HIGH encoding begins.
//   3. Measure HIGH duration   → compare against threshold.
//
// This two-step wait makes readTimedBit() robust against misinterpreting
// a lingering HIGH (e.g. from a device response phase) as bit-1 encoding.
int bitbang_readTimedBit(gpio_num_t pin, int64_t thresholdUs, int64_t timeoutUs) {
    // Step 1: Synchronise to the start of the bit's LOW period.
    // If the line is already LOW, this returns immediately.
    int64_t t = bitbang_waitForLevel(pin, 0, timeoutUs);
    if (t < 0) {
        ESP_LOGW(BITBANG_TAG, "readTimedBit: timeout waiting for LOW (bit start)");
        return -1;
    }

    // Step 2: Wait for rising edge — LOW period ended, encoding begins.
    t = bitbang_waitForLevel(pin, 1, timeoutUs);
    if (t < 0) {
        ESP_LOGW(BITBANG_TAG, "readTimedBit: timeout waiting for rising edge");
        return -1;
    }

    // Step 3: Measure high-pulse duration (encodes the bit value).
    int64_t start = esp_timer_get_time();
    t = bitbang_waitForLevel(pin, 0, timeoutUs);
    if (t < 0) {
        ESP_LOGW(BITBANG_TAG, "readTimedBit: timeout waiting for falling edge");
        return -1;
    }

    int64_t highUs = esp_timer_get_time() - start;
    int bit = (highUs > thresholdUs) ? 1 : 0;

    ESP_LOGD(BITBANG_TAG, "readTimedBit: high=%lld µs → bit=%d (thresh=%lld)",
             highUs, bit, thresholdUs);
    return bit;
}

// --- captureWave -------------------------------------------------------------
void bitbang_captureWave(gpio_num_t pin, int* buffer, int len, int intervalUs) {
    for (int i = 0; i < len; i++) {
        buffer[i] = gpio_get_level(pin);
        ets_delay_us((uint32_t)intervalUs);
    }
}
