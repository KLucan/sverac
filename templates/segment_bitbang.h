// segment_bitbang.h — Reusable bit-banging helpers for digital protocols
// =============================================================================
// INCLUDE this header in any driver that needs manual GPIO bit-banging
// (e.g. custom 1-wire protocols, DHT22-style sensors, etc.)
//
// Provides:
//   - waitForLevel()    — Block until a pin reaches a given level (with timeout).
//   - waitForLevelUs()  — Same, but timeout in µs.
//   - pulseOut()        — Output a timed LOW or HIGH pulse.
//   - readTimedBit()    — Read a single bit encoded as pulse-width.
//   - captureWave()     — Sample the pin at fixed interval into a buffer.
//
// DEPENDENCIES (must be included before this header or in your .cpp):
//   #include "driver/gpio.h"        // gpio_num_t, gpio_set_level, gpio_get_level
//   #include "esp_timer.h"          // esp_timer_get_time()
//   #include "rom/ets_sys.h"        // ets_delay_us()
//   #include "esp_log.h"            // ESP_LOGx macros
//
// USAGE:
//   1. #include "segment_bitbang.h" in your driver's .cpp file.
//   2. Call the functions directly (they are NOT class methods).
//   3. Customize BITBANG_DEFAULT_TIMEOUT_US as needed.
// =============================================================================

#ifndef _SEGMENT_BITBANG_H_
#define _SEGMENT_BITBANG_H_

#include "driver/gpio.h"        // gpio_num_t
#include <stdint.h>             // int64_t

// --- DEFAULT TIMEOUT (microseconds) ------------------------------------------
#ifndef BITBANG_DEFAULT_TIMEOUT_US
#define BITBANG_DEFAULT_TIMEOUT_US  200
#endif

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// waitForLevel() — Spin-wait until the pin reads 'level' (0 or 1).
//   @param pin        GPIO pin number (gpio_num_t)
//   @param level      Desired level (0 or 1)
//   @param timeoutUs  Max wait in microseconds (0 = use default)
//   @return           Elapsed µs, or -1 on timeout.
//
//   NOTE: Uses ets_delay_us(1) for ~1 µs polling resolution.
//         Not suitable for sub-microsecond precision.
// -----------------------------------------------------------------------------
int64_t bitbang_waitForLevel(gpio_num_t pin, int level, int64_t timeoutUs);

// -----------------------------------------------------------------------------
// pulseOut() — Drive the pin to 'level' for 'durationUs' microseconds.
//   The pin must already be configured as OUTPUT.
//   @param pin        GPIO pin number
//   @param level      Level to output (0 or 1)
//   @param durationUs Duration in µs
//
//   NOTE: Uses ets_delay_us() which is a busy-wait — blocks the CPU.
// -----------------------------------------------------------------------------
void bitbang_pulseOut(gpio_num_t pin, int level, int64_t durationUs);

// -----------------------------------------------------------------------------
// readTimedBit() — Read a single pulse-width-encoded bit.
//   Protocol: each bit starts with a LOW period, then a HIGH pulse whose
//   duration encodes the value (short → 0, long → 1).
//
//   Synchronisation:
//     1. Wait for LOW      → align with start of bit's LOW period.
//        (Returns immediately if line is already LOW.)
//     2. Wait for HIGH     → LOW period ended, encoding begins.
//     3. Measure HIGH duration → compare against threshold.
//
//   This two-step wait prevents misinterpreting a lingering HIGH signal
//   (e.g. from a device response phase) as a bit-1 encoding.
//
//   @param pin        GPIO pin (must be INPUT)
//   @param thresholdUs  Pulse width above which bit = 1
//   @param timeoutUs    Max wait for each edge (0 = use default)
//   @return             0 or 1, or -1 on timeout/error.
// -----------------------------------------------------------------------------
int bitbang_readTimedBit(gpio_num_t pin, int64_t thresholdUs, int64_t timeoutUs);

// -----------------------------------------------------------------------------
// captureWave() — Sample pin level into buffer at fixed intervals.
//   @param pin        GPIO pin (must be INPUT)
//   @param buffer     Output buffer (caller-allocated)
//   @param len        Number of samples
//   @param intervalUs Sampling interval in µs
// -----------------------------------------------------------------------------
void bitbang_captureWave(gpio_num_t pin, int* buffer, int len, int intervalUs);

#ifdef __cplusplus
}
#endif

#endif // _SEGMENT_BITBANG_H_
