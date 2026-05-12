// driver_digital_comms.cpp — Template for digital communication protocol drivers
// =============================================================================
// Fill in the stubs labelled "TODO" with your device-specific protocol logic.
// See driver_digital_comms.h for the full instructions.
// =============================================================================

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"           // esp_timer_get_time()
#include "rom/ets_sys.h"         // ets_delay_us()
#include "__DeviceName__.h"       // TODO: rename to your header

// --- CONSTRUCTOR -------------------------------------------------------------
__DeviceName__::__DeviceName__(int port) {
    m_pinNumber  = (gpio_num_t)port;
    m_lastReadUs = 0;
    LogName      = "__DeviceName__";    // TODO: set your log tag

    ESP_LOGI(LogName, "Configure port [%d] as bi-directional (open-drain)", port);

    // Reset pin
    gpio_reset_pin(m_pinNumber);

    // Start as INPUT (high-impedance) — the pull-up holds the line HIGH (idle)
    gpio_set_direction(m_pinNumber, GPIO_MODE_INPUT);
    gpio_set_pull_mode(m_pinNumber, GPIO_PULLUP_ONLY);

    // TODO: Add any device-specific initialisation here.
    //       e.g. wait 1 second for DHT22 stabilization:
    //       vTaskDelay(pdMS_TO_TICKS(1000));
}

// --- WAIT FOR LEVEL ----------------------------------------------------------
int64_t __DeviceName__::waitForLevel(int level, int64_t timeoutUs) {
    int64_t start = esp_timer_get_time();
    while (gpio_get_level(m_pinNumber) != level) {
        ets_delay_us(1);
        int64_t elapsed = esp_timer_get_time() - start;
        if (elapsed > timeoutUs) {
            return -1;   // timeout
        }
    }
    return esp_timer_get_time() - start;  // elapsed µs
}

// --- READ BIT ----------------------------------------------------------------
// Reads one bit from a pulse-width-encoded protocol (e.g. DHT22, 1-Wire).
//
// Protocol assumption:
//   Each bit starts with a LOW period (e.g. 50 µs), then a HIGH pulse whose
//   duration determines the bit value:
//     - short HIGH  (< threshold) → bit 0
//     - long HIGH   (> threshold) → bit 1
//
// Synchronisation strategy:
//   1. Wait for FALLING edge  → align with start of bit's LOW period.
//      (Returns immediately if line is already LOW from previous bit.)
//   2. Wait for RISING  edge  → LOW period ended, HIGH encoding begins.
//   3. Measure HIGH duration   → compare against threshold.
//
// This two-step wait makes readBit() robust against misinterpreting a
// lingering HIGH (e.g. from the device response phase) as bit-1 encoding.
int __DeviceName__::readBit() {
    // Step 1: Synchronise to the start of the bit's LOW period.
    // If the line is already LOW (common between consecutive bits),
    // this returns immediately.
    int64_t t = waitForLevel(0);
    if (t < 0) {
        ESP_LOGW(LogName, "readBit: timeout waiting for LOW (bit start)");
        return -1;
    }

    // Step 2: Wait for the rising edge — LOW period ended, encoding begins.
    t = waitForLevel(1);
    if (t < 0) {
        ESP_LOGW(LogName, "readBit: timeout waiting for rising edge");
        return -1;
    }

    // Step 3: Measure the HIGH pulse duration (encodes the bit value).
    int64_t start = esp_timer_get_time();
    t = waitForLevel(0);
    if (t < 0) {
        ESP_LOGW(LogName, "readBit: timeout waiting for falling edge");
        return -1;
    }

    int64_t highUs = esp_timer_get_time() - start;
    int bit = (highUs > __DEVICE_NAME___BIT_THRESHOLD_US) ? 1 : 0;

    ESP_LOGD(LogName, "readBit: high=%lld µs → bit=%d", highUs, bit);
    return bit;
}

// --- CAPTURE WAVE (debug helper) ---------------------------------------------
void __DeviceName__::captureWave(int* buffer, int len, int intervalUs) {
    for (int i = 0; i < len; i++) {
        buffer[i] = gpio_get_level(m_pinNumber);
        ets_delay_us(intervalUs);
    }
}

// --- PRINT WAVE (debug helper) -----------------------------------------------
void __DeviceName__::printWave() {
    const int WAVE_LEN = 1024;
    int wave[WAVE_LEN] = {0};

    // --- Send start signal ---
    gpio_set_direction(m_pinNumber, GPIO_MODE_OUTPUT);
    gpio_set_level(m_pinNumber, 0);
    ets_delay_us(__DEVICE_NAME___START_LOW_US);
    gpio_set_level(m_pinNumber, 1);
    gpio_set_direction(m_pinNumber, GPIO_MODE_INPUT);

    // --- Capture ---
    captureWave(wave, WAVE_LEN, __DEVICE_NAME___SAMPLE_INTERVAL_US);

    // --- Print (8 samples per line) ---
    ESP_LOGI(LogName, "--- WAVEFORM (interval=%d µs) ---",
             __DEVICE_NAME___SAMPLE_INTERVAL_US);
    for (int i = 0; i < WAVE_LEN; i += 8) {
        ESP_LOGI(LogName, "%d %d %d %d %d %d %d %d",
                 wave[i],   wave[i+1], wave[i+2], wave[i+3],
                 wave[i+4], wave[i+5], wave[i+6], wave[i+7]);
    }
    ESP_LOGI(LogName, "--- END WAVEFORM ---");
}

// --- GET READING -------------------------------------------------------------
__device_name___reading_t __DeviceName__::getReading() {
    __device_name___reading_t reading = {};

    // --- Rate limiting ---
    // TODO: Uncomment if your device has a minimum read interval.
    // int64_t now = esp_timer_get_time();
    // if ((now - m_lastReadUs) < __DEVICE_NAME___READ_DELAY_US) {
    //     ESP_LOGW(LogName, "Read too fast — skipping");
    //     return reading;
    // }
    // m_lastReadUs = now;

    // =========================================================================
    // STEP 1: Send start signal
    // =========================================================================
    // TODO: Adjust timing to match your device's datasheet.

    // Pull line LOW for START_LOW_US to initiate communication
    gpio_set_direction(m_pinNumber, GPIO_MODE_OUTPUT);
    gpio_set_level(m_pinNumber, 0);
    ets_delay_us(__DEVICE_NAME___START_LOW_US);

    // Release line (pull HIGH) and switch to INPUT to listen for response
    gpio_set_level(m_pinNumber, 1);
    ets_delay_us(__DEVICE_NAME___START_HIGH_US);
    gpio_set_direction(m_pinNumber, GPIO_MODE_INPUT);

    // =========================================================================
    // STEP 2: Wait for device response
    // =========================================================================
    // TODO: Your device may send a LOW pulse followed by a HIGH pulse.
    //       The DHT22 sends: 80 µs LOW (response) then 80 µs HIGH (ready).

    // Wait for device to pull LOW
    int64_t t = waitForLevel(0, 200);   // 200 µs timeout
    if (t < 0) {
        ESP_LOGW(LogName, "No response from device (timeout waiting for LOW)");
        return reading;
    }
    ESP_LOGD(LogName, "Device response LOW: %lld µs", t);

    // Wait for device to release (pull HIGH)
    t = waitForLevel(1, 200);
    if (t < 0) {
        ESP_LOGW(LogName, "No response from device (timeout waiting for HIGH)");
        return reading;
    }
    ESP_LOGD(LogName, "Device response HIGH: %lld µs", t);

    // =========================================================================
    // STEP 3: Sync to first data bit
    // =========================================================================
    // After the device's response/preparation HIGH, most protocols pull the
    // line LOW to signal the start of the first data bit.  If we jump straight
    // into readBit(), the initial waitForLevel(1) may immediately see the
    // line still HIGH from the response phase and misinterpret the remaining
    // response HIGH time as the first bit's encoding.
    //
    // Therefore, wait for the falling edge (HIGH → LOW) here to align with
    // the start of the first bit before entering the bit-reading loop.
    // Uncomment and adjust the timeout for your device:
    //
    // t = waitForLevel(0, 200);
    // if (t < 0) {
    //     ESP_LOGW(LogName, "No response from device (timeout waiting for first bit)");
    //     return reading;
    // }

    // =========================================================================
    // STEP 4: Read data bits
    // =========================================================================
    // TODO: Adjust frame size and bit reading for your device.
    //
    // After the synchronising waitForLevel(0) above (or if your protocol
    // does not need it), the line is at the LOW start of the first bit.
    // readBit() first calls waitForLevel(0) — returns immediately if already
    // LOW — then waitForLevel(1) to measure the encoding pulse.

    for (int i = 0; i < __DEVICE_NAME___NUM_BYTES; i++) {
        reading.data[i] = 0;
    }

    int bitIndex = 0;
    for (int byteIdx = 0; byteIdx < __DEVICE_NAME___NUM_BYTES; byteIdx++) {
        for (int bit = 7; bit >= 0; bit--) {   // MSB first
            int b = readBit();
            if (b < 0) {
                ESP_LOGW(LogName, "Failed to read bit %d", bitIndex);
                return reading;   // invalid
            }
            reading.data[byteIdx] |= (b << bit);
            bitIndex++;
        }
    }

    ESP_LOGI(LogName, "Read %d bits: [%02X %02X %02X %02X %02X]",
             bitIndex,
             reading.data[0], reading.data[1], reading.data[2],
             reading.data[3], reading.data[4]);

    // =========================================================================
    // STEP 5: Validate checksum / CRC
    // =========================================================================
    // TODO: Implement checksum validation for your device.
    //
    // Example for DHT22 (last byte = sum of first 4 bytes, masked to 8 bits):
    // uint8_t sum = reading.data[0] + reading.data[1]
    //             + reading.data[2] + reading.data[3];
    // if (sum == reading.data[4]) {
    //     reading.valid = true;
    // } else {
    //     ESP_LOGW(LogName, "Checksum mismatch: calc=%02X, recv=%02X",
    //              sum, reading.data[4]);
    //     reading.valid = false;
    // }

    // =========================================================================
    // STEP 6: Parse values from raw bytes
    // =========================================================================
    // TODO: Convert raw bytes to physical values per your device's datasheet.
    //
    // Example for DHT22:
    // reading.humidity    = ((int16_t)(reading.data[0] << 8) | reading.data[1]) / 10.0f;
    // reading.temperature = ((int16_t)(reading.data[2] << 8) | reading.data[3]) / 10.0f;
    // reading.checksum    = reading.data[4];

    // Set pin back to INPUT with pull-up (idle state)
    gpio_set_direction(m_pinNumber, GPIO_MODE_INPUT);

    reading.valid = true;   // TODO: set based on checksum result
    return reading;
}
