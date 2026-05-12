// driver_analog.cpp — Template for analog sensor drivers (ADC oneshot mode)
// =============================================================================
// Fill in the stubs labelled "TODO" with your device-specific logic.
// Esp. the convertToSensor() method at the bottom.
// See driver_analog.h for the full instructions.
// =============================================================================

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "hal/adc_types.h"
#include "math.h"               // logf, powf, etc.
#include "__DeviceName__.h"      // TODO: rename to your header

// --- CONSTRUCTOR -------------------------------------------------------------
__DeviceName__::__DeviceName__(int channel) {
    m_unit       = __DEVICE_NAME___ADC_UNIT;
    m_channel    = (channel >= 0) ? (adc_channel_t)channel : __DEVICE_NAME___ADC_CHANNEL;
    m_adcHandle  = nullptr;
    m_caliHandle = nullptr;
    m_calibrated = false;
    LogName      = "__DeviceName__";   // TODO: set your log tag

    ESP_LOGI(LogName, "Initialising ADC Unit %d, Channel %d",
             (int)m_unit, (int)m_channel);

    // --- Step 1: Create ADC oneshot unit -------------------------------------
    adc_oneshot_unit_init_cfg_t initCfg = {};
    initCfg.unit_id  = m_unit;
    initCfg.ulp_mode = ADC_ULP_MODE_DISABLE;
    // initCfg.clk_src = 0;  // default clock source

    esp_err_t ret = adc_oneshot_new_unit(&initCfg, &m_adcHandle);
    if (ret != ESP_OK) {
        ESP_LOGE(LogName, "adc_oneshot_new_unit failed: %s", esp_err_to_name(ret));
        return;
    }

    // --- Step 2: Configure the ADC channel -----------------------------------
    adc_oneshot_chan_cfg_t chanCfg = {};
    chanCfg.atten    = __DEVICE_NAME___ADC_ATTEN;
    chanCfg.bitwidth = __DEVICE_NAME___ADC_BITWIDTH;

    ret = adc_oneshot_config_channel(m_adcHandle, m_channel, &chanCfg);
    if (ret != ESP_OK) {
        ESP_LOGE(LogName, "adc_oneshot_config_channel failed: %s", esp_err_to_name(ret));
        return;
    }

    // --- Step 3: Create calibration scheme (Line Fitting) ---------------------
    adc_cali_line_fitting_config_t caliCfg = {};
    caliCfg.unit_id  = m_unit;
    caliCfg.atten    = __DEVICE_NAME___ADC_ATTEN;
    caliCfg.bitwidth = __DEVICE_NAME___ADC_BITWIDTH;
    // caliCfg.default_vref = 0;  // Use eFuse values if available

    ret = adc_cali_create_scheme_line_fitting(&caliCfg, &m_caliHandle);
    if (ret == ESP_OK) {
        m_calibrated = true;
        ESP_LOGI(LogName, "ADC calibration scheme created (Line Fitting)");
    } else {
        ESP_LOGW(LogName, "ADC calibration not available (%s) — raw only",
                 esp_err_to_name(ret));
    }

    ESP_LOGI(LogName, "ADC initialisation complete");
}

// --- DESTRUCTOR --------------------------------------------------------------
__DeviceName__::~__DeviceName__() {
    if (m_caliHandle != nullptr) {
        adc_cali_delete_scheme_line_fitting(m_caliHandle);
        m_caliHandle = nullptr;
    }
    if (m_adcHandle != nullptr) {
        adc_oneshot_del_unit(m_adcHandle);
        m_adcHandle = nullptr;
    }
    ESP_LOGI(LogName, "ADC deinitialised");
}

// --- GET RAW -----------------------------------------------------------------
int __DeviceName__::getRaw() {
    int raw = 0;
    esp_err_t ret = adc_oneshot_read(m_adcHandle, m_channel, &raw);
    if (ret != ESP_OK) {
        ESP_LOGE(LogName, "adc_oneshot_read failed: %s", esp_err_to_name(ret));
        return -1;
    }
    return raw;
}

// --- GET VOLTAGE (mV) --------------------------------------------------------
int __DeviceName__::getVoltageMv() {
    int raw = getRaw();
    if (raw < 0) return -1;

    if (!m_calibrated) {
        // Fallback: manual conversion. Vout = Dout * Vmax / Dmax
        // For 12-bit, 12 dB attenuation: Vmax ≈ 3.3 V, Dmax = 4095
        return (int)((float)raw * 3300.0f / 4095.0f);
    }

    int voltage_mv = 0;
    esp_err_t ret = adc_cali_raw_to_voltage(m_caliHandle, raw, &voltage_mv);
    if (ret != ESP_OK) {
        ESP_LOGE(LogName, "adc_cali_raw_to_voltage failed: %s", esp_err_to_name(ret));
        return -1;
    }
    return voltage_mv;
}

// --- GET READING (single sample) ---------------------------------------------
__device_name___reading_t __DeviceName__::getReading() {
    __device_name___reading_t reading = {};
    reading.valid = false;

    reading.raw = getRaw();
    if (reading.raw < 0) return reading;

    reading.voltage_mv = getVoltageMv();
    if (reading.voltage_mv < 0) return reading;

    reading.voltage = (float)reading.voltage_mv / 1000.0f;

    // --- Device-specific conversion ------------------------------------------
    // TODO: Uncomment and implement your formula.
    // reading.temperature = convertToSensor(reading.voltage);
    // reading.resistance  = ... ;
    // reading.humidity    = ... ;

    reading.valid = true;
    return reading;
}

// --- GET READING MULTI (averaged) --------------------------------------------
__device_name___reading_t __DeviceName__::getReadingMulti(int samples) {
    if (samples < 1) samples = 1;

    int64_t sumRaw    = 0;
    int64_t sumMv     = 0;
    int     validCount = 0;

    for (int i = 0; i < samples; i++) {
        int raw = getRaw();
        if (raw < 0) continue;

        int mv = 0;
        if (m_calibrated) {
            adc_cali_raw_to_voltage(m_caliHandle, raw, &mv);
        } else {
            mv = (int)((float)raw * 3300.0f / 4095.0f);
        }

        sumRaw += raw;
        sumMv  += mv;
        validCount++;

        // Small delay between samples to let ADC settle
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    __device_name___reading_t reading = {};
    if (validCount == 0) return reading;

    reading.raw        = (int)(sumRaw / validCount);
    reading.voltage_mv = (int)(sumMv  / validCount);
    reading.voltage    = (float)reading.voltage_mv / 1000.0f;

    // --- Device-specific conversion ------------------------------------------
    // TODO: Uncomment and implement.
    // reading.temperature = convertToSensor(reading.voltage);

    reading.valid = true;

    ESP_LOGI(LogName, "Multi-reading (%d samples): Raw=%d, Voltage=%d mV (%.3f V)",
             validCount, reading.raw, reading.voltage_mv, (double)reading.voltage);

    return reading;
}

// --- CONVERT TO SENSOR VALUE -------------------------------------------------
// TODO: Implement this based on your sensor's datasheet.
//
// *** EXAMPLE 1: NTC Thermistor (VMA320) with voltage divider ***
//    VCC = 3.3 V,  R_fixed = 10 kΩ
//    Rt = R_fixed * Vout / (VCC - Vout)
//    Steinhart-Hart:  1/T = A + B*ln(Rt) + C*(ln(Rt))^3   (T in Kelvin)
//    T_celsius = T_kelvin - 273.15
//
//    float __DeviceName__::convertToSensor(float voltage_v) {
//        if (voltage_v >= __DEVICE_NAME___VCC) return -999.0f;  // avoid div by zero
//        float Rt = __DEVICE_NAME___R_FIXED * voltage_v / (__DEVICE_NAME___VCC - voltage_v);
//        float logRt = logf(Rt);
//        float invT = __DEVICE_NAME___A_COEFF
//                   + __DEVICE_NAME___B_COEFF * logRt
//                   + __DEVICE_NAME___C_COEFF * logRt * logRt * logRt;
//        float T_kelvin = 1.0f / invT;
//        return T_kelvin - __DEVICE_NAME___KELVIN_ZERO;
//    }
//
// *** EXAMPLE 2: Linear sensor (e.g. potentiometer, 0–100%) ***
//    float __DeviceName__::convertToSensor(float voltage_v) {
//        return voltage_v / __DEVICE_NAME___VCC * 100.0f;  // percentage
//    }
//
// *** EXAMPLE 3: Simple linear mapping (e.g. 0–5 V → 0–100 units) ***
//    float __DeviceName__::convertToSensor(float voltage_v) {
//        return voltage_v * (100.0f / 5.0f);
//    }

float __DeviceName__::convertToSensor(float voltage_v) {
    // TODO: Replace with your sensor-specific conversion formula.
    // (See examples above.)
    ESP_LOGW(LogName, "convertToSensor() not implemented — returning voltage");
    return voltage_v;   // placeholder
}
