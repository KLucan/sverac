#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "sdkconfig.h"

#include "CDS1307.h"

static const char *TAG = "MAIN";
static CDS1307 *g_rtc = nullptr;

#define I2C_PORT I2C_NUM_0
#define PIN_SDA GPIO_NUM_21
#define PIN_SCL GPIO_NUM_22
#define I2C_CLK_SPEED 100000 // 100 kHz

#define INIT_YEAR 26 // 2026
#define INIT_MONTH 5
#define INIT_DATE 12
#define INIT_DOW 2 // utorak
#define INIT_HOUR 12
#define INIT_MIN 0
#define INIT_SEC 0

#define PIN_BUTTON GPIO_NUM_13

static bool rubGumb()
{
    static bool last = 1;
    bool sad = gpio_get_level(PIN_BUTTON);
    bool rub = (last == 0 && sad == 1);
    last = sad;
    return rub;
}

extern "C" void app_main(void)
{

    g_rtc = new CDS1307(I2C_PORT, PIN_SDA, PIN_SCL, I2C_CLK_SPEED);
    if (!g_rtc || !g_rtc->isInitialised())
    {
        ESP_LOGE(TAG, "DS1307 INIT ERROR");
        if (g_rtc)
            delete g_rtc;
        return;
    }

    if (!g_rtc->isConnected())
    {
        ESP_LOGE(TAG, "DS1307 BEZ SIGNALA - provjeri žice");
        delete g_rtc;
        return;
    }
    ESP_LOGI(TAG, "DS1307 INIT OK");

    if (g_rtc->isClockHalted())
    {
        g_rtc->setClockHalt(false);
    }

    ds1307_time_t time = {0};
    time.year = INIT_YEAR;
    time.month = INIT_MONTH;
    time.date = INIT_DATE;
    time.dayOfWeek = INIT_DOW;
    time.hours = INIT_HOUR;
    time.minutes = INIT_MIN;
    time.seconds = INIT_SEC;
    time.valid = true;

    g_rtc->setTime(time);

    // gumb
    gpio_reset_pin(PIN_BUTTON);
    gpio_set_direction(PIN_BUTTON, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_BUTTON, GPIO_PULLUP_ONLY);

    ESP_LOGI(TAG, "INIT OK, gumb GPIO: %d", PIN_BUTTON);

    while (1)
    {
        if (rubGumb())
        {
            ds1307_time_t t = g_rtc->getTime();

            if (t.valid)
            {
                ESP_LOGI(TAG, "DATUM : 20%02d-%02d-%02d", t.year, t.month, t.date);
                ESP_LOGI(TAG, "VRIJEME : %02d:%02d:%02d", t.hours, t.minutes, t.seconds);
            }
            else
            {
                ESP_LOGW(TAG, "DS1307 READ ERROR");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20)); // ~50 Hz
    }
}