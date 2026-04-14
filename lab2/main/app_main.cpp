#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "DHT22.h"
#include "VMA320.h"

#define BUTTON_GPIO GPIO_NUM_13
#define DHT_GPIO GPIO_NUM_15
#define VMA_CHANNEL ADC1_CHANNEL_6
#define RETRY_COUNT 5

static const char *TAG = "MAIN";
static TaskHandle_t button_task_handle = NULL;

static void IRAM_ATTR button_pressed_isr(void *arg)
{
    gpio_intr_disable(BUTTON_GPIO);

    BaseType_t high_task_wakeup = pdFALSE;
    vTaskNotifyGiveFromISR(button_task_handle, &high_task_wakeup);

    if (high_task_wakeup) {
        portYIELD_FROM_ISR();
    }
}

static void button_task(void *arg)
{
    DHT22 *dht = new DHT22(DHT_GPIO);
    VMA320 *vma = new VMA320(VMA_CHANNEL);
    dht22_reading_t dht_reading = {0};
    vma320_reading_t vma_reading = {0};

    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        ESP_LOGI(TAG, "Button pressed");
        for (int i = 0; i<RETRY_COUNT;i++){
            dht_reading = dht->getReading();
            if (dht_reading.valid) break;
        }
        vma_reading = vma->getReading();
        ESP_LOGI(TAG, "DHT22 temperature: %.2f C", dht_reading.temperature);
        ESP_LOGI(TAG, "VMA320 temperature: %.2f C", vma_reading.temperature);
        vTaskDelay(pdMS_TO_TICKS(200));
        gpio_intr_enable(BUTTON_GPIO);
    }
}

extern "C" void app_main(void)
{
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << BUTTON_GPIO);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    gpio_config(&io_conf);

    xTaskCreate(button_task, "button_task", 8192, NULL, 10, &button_task_handle);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_pressed_isr, NULL);
}