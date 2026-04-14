#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"
#include "driver/adc.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "math.h"
#include "VMA320.h"

static adc_oneshot_unit_handle_t adc_handle = NULL;
static adc_cali_handle_t cali_handle = NULL;

VMA320::VMA320(int channel){
    m_channel = (adc_channel_t)channel;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));
    
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle));
    
    ESP_LOGI(LogName, "Configured ADC channel %d (ATTEN_DB_11)", channel);
}

vma320_reading_t VMA320::getReading() {
    vma320_reading_t reading = {};
    
    int raw;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, m_channel, &raw));
    
    int voltage_mv;
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw, &voltage_mv));
    
    float vout = (float)voltage_mv / 1000.0f;
    
    if ((VCC - vout) != 0) {
        float Rt = Rkonst * vout / (VCC - vout);
        
        ESP_LOGI(LogName, "Vout=%.3f V Rt=%.0f Ohm", vout, Rt);

        float logRt = logf(Rt);
        float inverse_T = A + B * logRt + C * logRt * logRt * logRt;
        reading.temperature = 1/inverse_T - KELVIN_ZERO;

    }

    ESP_LOGI(LogName, "Raw=%d Voltage=%d mV; Temp=%.2f", raw, voltage_mv, reading.temperature);
    return reading;
}