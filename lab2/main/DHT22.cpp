#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"
#include "DHT22.h"

DHT22::DHT22(int port){
    //Config Port on constructor
    m_pinNumber = (gpio_num_t)port;
    ESP_LOGI(LogName, "Configure port[%d] to I/O!!!", port);
    gpio_reset_pin(m_pinNumber);
    /* Set the GPIO as I/O */
    gpio_set_pull_mode(m_pinNumber, GPIO_PULLUP_ONLY);
}

void DHT22::waitForLevel(uint32_t level) {
    int64_t start_time = esp_timer_get_time();
    while (gpio_get_level(m_pinNumber) != level) {
        int64_t time = esp_timer_get_time();
        ets_delay_us(1);
        if (time - start_time > 70) return;
        //ESP_LOGI(LogName, "time %lld, level %d", time, level);
    }
}

bool DHT22::getBit() {
    waitForLevel(1);                  // rising edge
    int64_t start = esp_timer_get_time();
    waitForLevel(0);                  // falling edge
    int64_t high_us = esp_timer_get_time() - start;
    ESP_LOGI(LogName, "high_us=%lld bit=%d", high_us, high_us > 40);
    return high_us > 40;
}

void DHT22::printWave(){
    int wave[1024] = {0};
    gpio_set_direction(m_pinNumber, GPIO_MODE_OUTPUT);
    gpio_set_level(m_pinNumber, 0);
    ets_delay_us(10000);
    gpio_set_level(m_pinNumber, 1);
    gpio_set_direction(m_pinNumber, GPIO_MODE_INPUT);
    for (int i = 0; i<1024;i++){
        wave[i] = gpio_get_level(m_pinNumber);
        ets_delay_us(5);
    }
    for (int i = 0; i<1024;i += 8) {
        ESP_LOGI(LogName, "%d %d %d %d %d %d %d %d", wave[i], wave[i+1], wave[i+2], wave[i+3], wave[i+4], wave[i+5], wave[i+6], wave[i+7]);
    }
}

dht22_reading_t DHT22::getReading() {
    dht22_reading_t reading = {};
    bool wave[1024] = {0};
    gpio_set_direction(m_pinNumber, GPIO_MODE_OUTPUT);
    gpio_set_level(m_pinNumber, 0);
    ets_delay_us(10000);
    gpio_set_level(m_pinNumber, 1);
    gpio_set_direction(m_pinNumber, GPIO_MODE_INPUT);
    for (int i = 0; i < 1024; i++) {
        wave[i] = gpio_get_level(m_pinNumber);
        ets_delay_us(5);
    }

    uint8_t data[5] = {0};
    int bitIndex = 0;
    int i = 0;

    
    while (i < 1024 && wave[i] == 1) {
        i++;
    }
    // DHT pulse 0
    while (i < 1024 && wave[i] == 0) {
        i++;
    }
    //DHT pulse 1
    while (i < 1024 && wave[i] == 1) {
        i++;
    }
    //Hands shook
    while (i < 1024 && bitIndex < 40) {
        int lowLen = 0;
        while (i < 1024 && wave[i] == 0) {
            lowLen++;
            i++;
        }

        int highLen = 0;
        while (i < 1024 && wave[i] == 1) {
            highLen++;
            i++;
        }

        if (highLen < 2) {
            continue;
        }

        if (highLen > 20) {
            ESP_LOGI(LogName, "End of frame, highLen=%d", highLen);
            break;
        }

        int bit = (highLen >= 5) ? 1 : 0;

        data[bitIndex / 8] <<= 1;
        data[bitIndex / 8] |= bit;
        bitIndex++;
    }

    if (bitIndex == 40) {
        reading.humidity = (float)data[0] + ((float)data[1] / 10.0f);
        reading.temperature = ((int16_t)(data[2] << 8) | data[3]) / 10.0f;
        reading.checksum = data[4];

        uint8_t sum = data[0] + data[1] + data[2] + data[3];
        reading.valid = (sum == data[4]);
    } else {
        reading.valid = false;
    }

    ESP_LOGI(LogName,
             "bits=%d RH=%.2f T=%.2f checksum=%u valid=%d",
             bitIndex,
             reading.humidity,
             reading.temperature,
             reading.checksum,
             reading.valid);

    return reading;
}