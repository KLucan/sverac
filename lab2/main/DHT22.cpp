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
        ets_delay_us(1);
    }
}

bool DHT22::getBit() {
    waitForLevel(0);                  // start of this bit
    waitForLevel(1);                  // rising edge
    int64_t start = esp_timer_get_time();
    waitForLevel(0);                  // falling edge
    int64_t high_us = esp_timer_get_time() - start;
    ESP_LOGI(LogName, "high_us=%lld bit=%d", high_us, high_us > 40);
    return high_us > 40;
}

dht22_reading_t DHT22::getReading() {
    dht22_reading_t reading = {};
    uint8_t data[5] = {0};
    ESP_LOGI(LogName, "Start read.");
    gpio_set_direction(m_pinNumber, GPIO_MODE_OUTPUT);
    gpio_set_level(m_pinNumber, 0);
    ets_delay_us(20000);
    gpio_set_level(m_pinNumber, 1);
    ets_delay_us(30);
    gpio_set_direction(m_pinNumber, GPIO_MODE_INPUT);

    ESP_LOGI(LogName, "After release level=%d", gpio_get_level(m_pinNumber));
    waitForLevel(0);
    ESP_LOGI(LogName, "Sensor pulled low");
    waitForLevel(1);
    ESP_LOGI(LogName, "Sensor pulled high");
    waitForLevel(0);
    ESP_LOGI(LogName, "Shook hands.");
    for (int i = 0; i < 40; i++) {
        bool bit = getBit();
        data[i / 8] <<= 1;
        data[i / 8] |= bit;
    }
    ESP_LOGI(LogName, "Read: %x %x %x %x %x", data[0], data[1], data[2], data[3], data[4]);
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    reading.valid = (sum == data[4]);

    return reading;
}