// driver_analog.h — Template for analog sensor drivers (ADC oneshot mode)
// =============================================================================
// Use this template for devices read via the ESP32 ADC:
//   - NTC thermistors, photoresistors, potentiometers, analog voltage outputs.
//
// PATTERN: Constructor initialises ADC oneshot unit + calibration.
//          getReading() performs a one-shot ADC read, converts raw → voltage,
//          then applies a device-specific formula (e.g. Steinhart-Hart for NTC,
//          voltage divider for LDR, linear scaling for potentiometers).
//          getReadingMulti() averages N samples to reduce noise.
//
// ESP-IDF v5.4 ADC ONESHOT API:
//   #include "esp_adc/adc_oneshot.h"
//   #include "esp_adc/adc_cali.h"
//   #include "esp_adc/adc_cali_scheme.h"
//   #include "hal/adc_types.h"
//
// INSTRUCTIONS:
//   1. Copy and rename (e.g. CMySensor.h).
//   2. Replace __DEVICE_NAME__ with your class name.
//   3. Set ADC unit (ADC_UNIT_1 / ADC_UNIT_2), channel, attenuation, bitwidth.
//   4. Implement the conversion formula in getReading().
//   5. Customize the reading struct with all relevant sensor values.
// =============================================================================

#ifndef _DRIVER_ANALOG_H_
#define _DRIVER_ANALOG_H_

#include "hal/adc_types.h"          // adc_channel_t, adc_unit_t, adc_atten_t, adc_bitwidth_t
#include "esp_adc/adc_oneshot.h"    // adc_oneshot_unit_handle_t
#include "esp_adc/adc_cali.h"       // adc_cali_handle_t
#include <stdint.h>

// --- ADC HARDWARE CONFIGURATION ----------------------------------------------
// TODO: Set these for your sensor and ESP32 board.
//       ADC1 channels: GPIO32–39  (ADC1_CH4–ADC1_CH0, ADC1_CH3)
//       ADC2 channels: GPIO0,2,4,12–15,25–27  (cannot coexist with Wi-Fi!)
#define __DEVICE_NAME___ADC_UNIT       ADC_UNIT_1       // ADC_UNIT_1 or ADC_UNIT_2
#define __DEVICE_NAME___ADC_CHANNEL    ADC_CHANNEL_6     // GPIO34 = ADC1_CH6
#define __DEVICE_NAME___ADC_ATTEN      ADC_ATTEN_DB_12   // 0 dB ≈ 0–1.1 V, 12 dB ≈ 0–3.3 V
#define __DEVICE_NAME___ADC_BITWIDTH   ADC_BITWIDTH_12   // 12-bit = 0–4095

// --- DEVICE-SPECIFIC CONSTANTS -----------------------------------------------
// TODO: Add constants used in your conversion formula.
//       Example for NTC thermistor:
//         #define __DEVICE_NAME___VCC         3.3f        // Supply voltage
//         #define __DEVICE_NAME___R_FIXED     10000.0f    // Fixed resistor in divider (Ω)
//         #define __DEVICE_NAME___A_COEFF     0.000143f   // Steinhart-Hart A
//         #define __DEVICE_NAME___B_COEFF     0.000389f   // Steinhart-Hart B
//         #define __DEVICE_NAME___C_COEFF     -5.63e-07f  // Steinhart-Hart C
//         #define __DEVICE_NAME___KELVIN_ZERO 273.15f

// --- MULTI-SAMPLING (for noise reduction) ------------------------------------
#define __DEVICE_NAME___DEFAULT_SAMPLES    8       // Number of samples to average

// --- READING STRUCT ----------------------------------------------------------
// TODO: Customize with the values your sensor produces.
typedef struct {
    int     raw;            // Raw ADC value (0–4095 for 12-bit)
    int     voltage_mv;     // Calibrated voltage in mV
    float   voltage;        // Voltage in V
    // float   resistance;     // For resistive sensors (NTC, LDR)
    // float   temperature;    // For temperature sensors
    // float   humidity;       // For humidity sensors
    bool    valid;          // Whether the reading was successful
} __device_name___reading_t;


// --- CLASS DECLARATION -------------------------------------------------------
class __DeviceName__ {

public:
    // -------------------------------------------------------------------------
    // Constructor — initialises ADC oneshot unit + calibration
    // @param channel  ADC channel (adc_channel_t, e.g. ADC_CHANNEL_6)
    //                 Overrides __DEVICE_NAME___ADC_CHANNEL if non-negative.
    // -------------------------------------------------------------------------
    __DeviceName__(int channel = -1);

    // -------------------------------------------------------------------------
    // Destructor — deinitialises ADC and cleans up
    // -------------------------------------------------------------------------
    ~__DeviceName__();

    // -------------------------------------------------------------------------
    // getReading() — Perform a single ADC read and convert to sensor units.
    // -------------------------------------------------------------------------
    __device_name___reading_t getReading();

    // -------------------------------------------------------------------------
    // getReadingMulti() — Average N samples to reduce noise.
    // @param samples  Number of samples (default: __DEVICE_NAME___DEFAULT_SAMPLES)
    // -------------------------------------------------------------------------
    __device_name___reading_t getReadingMulti(int samples = __DEVICE_NAME___DEFAULT_SAMPLES);

    // -------------------------------------------------------------------------
    // getRaw() — Return raw ADC value only (fast, no conversion).
    // -------------------------------------------------------------------------
    int getRaw();

    // -------------------------------------------------------------------------
    // getVoltageMv() — Return calibrated voltage in mV.
    // -------------------------------------------------------------------------
    int getVoltageMv();

private:
    adc_unit_t              m_unit;             // ADC unit (1 or 2)
    adc_channel_t           m_channel;          // ADC channel
    adc_oneshot_unit_handle_t m_adcHandle;      // ADC oneshot handle
    adc_cali_handle_t       m_caliHandle;       // ADC calibration handle
    bool                    m_calibrated;       // Whether calibration is active
    const char*             LogName;            // Tag for ESP_LOGx macros

    // -------------------------------------------------------------------------
    // convertToSensor() — Device-specific conversion from voltage to sensor value.
    //                     TODO: Implement this based on your sensor's datasheet.
    // -------------------------------------------------------------------------
    float convertToSensor(float voltage_v);
};

#endif // _DRIVER_ANALOG_H_
