// main_task.cpp — Template: Main program using FreeRTOS tasks
// =============================================================================
// Use this template when your application needs:
//   - Periodic polling of multiple drivers (each driver has a tick() method).
//   - Parallel execution: one task for sensor polling, another for comms, etc.
//   - Blocking delay between sensor reads without blocking the main loop.
//
// PATTERN:
//   app_main() creates driver objects, spawns a worker task, then enters its
//   own event loop (or sleeps forever).  The worker task calls tick() on each
//   driver periodically.  Callbacks from input drivers fire in the task context
//   and can change the state of other drivers.
//
// FreeRTOS API used (ESP-IDF v5.4):
//   xTaskCreate(), vTaskDelay(), pdMS_TO_TICKS(), TaskHandle_t
//
// INSTRUCTIONS:
//   1. Copy this file to your project's main/ directory as app_main.cpp.
//   2. Add your driver headers in the #include section.
//   3. Define GPIO pins and device parameters.
//   4. Create driver objects in app_main() (before task creation).
//   5. Wire up callbacks.
//   6. In the task function, call driver->tick() or driver->getReading()
//      at the desired interval.
//   7. Adjust task stack size and priority as needed.
// =============================================================================

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "sdkconfig.h"

// --- DRIVER HEADERS ----------------------------------------------------------
// TODO: Include your driver headers here.
// #include "CLed.h"
// #include "CButton.h"
// #include "CDHT22.h"
// #include "CVMA320.h"

// --- LOG TAG ----------------------------------------------------------------
static const char* TAG = "MAIN";

// --- GPIO PIN DEFINITIONS ----------------------------------------------------
// TODO: Set your GPIO pins here (or in each driver's header).
#define PIN_LED      GPIO_NUM_2     // On-board LED on many ESP32 dev boards
#define PIN_BUTTON   GPIO_NUM_0     // BOOT button (active low, pulled up)
// #define PIN_DHT22    GPIO_NUM_15
// #define PIN_NTC      GPIO_NUM_34   // ADC1_CH6

// --- FORWARD DECLARATIONS ----------------------------------------------------
// TODO: Add extern declarations for globally accessed driver objects.
// extern CLed*    g_led;
// extern CButton* g_button;

// =============================================================================
// TASK PARAMETERS STRUCT
// =============================================================================
// Bundles pointers to driver objects that the task needs to tick/read.
// TODO: Add/remove fields to match your driver objects.
struct TaskParams {
    // CLed*     led;
    // CButton*  button;
    // CDHT22*   dht22;
    // CVMA320*  ntc;

    int       periodMs;   // Tick period in milliseconds
};

// =============================================================================
// WORKER TASK — Called by FreeRTOS, runs forever
// =============================================================================
// This task periodically calls tick() on each driver and/or performs readings.
// TODO: Customize to your needs.
void worker_task(void* parameters) {
    ESP_LOGI(TAG, "Worker task started");

    // --- Cast parameters ---
    TaskParams* params = (TaskParams*)parameters;
    // CLed*     led    = params->led;
    // CButton*  button = params->button;
    // CDHT22*   dht22  = params->dht22;
    // CVMA320*  ntc    = params->ntc;
    int periodMs = params->periodMs;

    // --- Optional: Initialisation that requires running FreeRTOS ---
    // e.g., waiting for device boot:
    // vTaskDelay(pdMS_TO_TICKS(2000));

    // --- Periodic loop ---
    TickType_t lastWakeTime = xTaskGetTickCount();

    while (1) {
        // =====================================================================
        // TICK ALL DRIVERS (for state-machine-based drivers)
        // =====================================================================
        // TODO: Call tick() on each driver that has timed behaviour.
        // led->tick();
        // button->tick();

        // =====================================================================
        // READ SENSORS (for polled drivers that don't use tick)
        // =====================================================================
        // TODO: Read and log sensor values at desired intervals.
        //
        // Example: Read temperature every 2 seconds
        // static int64_t lastSensorRead = 0;
        // int64_t now = esp_timer_get_time();
        // if ((now - lastSensorRead) > 2000000LL) {   // 2 seconds
        //     lastSensorRead = now;
        //
        //     dht22_reading_t dht = dht22->getReading();
        //     if (dht.valid) {
        //         ESP_LOGI(TAG, "DHT22: T=%.1f°C  RH=%.1f%%",
        //                  dht.temperature, dht.humidity);
        //     }
        //
        //     vma320_reading_t ntcReading = ntc->getReadingMulti(16);
        //     if (ntcReading.valid) {
        //         ESP_LOGI(TAG, "NTC: T=%.2f°C (raw=%d, %d mV)",
        //                  ntcReading.temperature,
        //                  ntcReading.raw, ntcReading.voltage_mv);
        //     }
        // }

        // =====================================================================
        // DELAY (periodic task pattern using vTaskDelayUntil)
        // =====================================================================
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(periodMs));
    }
}

// =============================================================================
// CALLBACK FUNCTIONS — Fired from input driver state machines
// =============================================================================
// These run in the context of the task that calls button->tick().
// TODO: Implement your callback functions here.
//
// Example:
// void onSingleClick() {
//     ESP_LOGI(TAG, "Single click — toggling LED");
//     LedStatus state = g_led->getState();
//     g_led->setState((state != BLINK) ? BLINK : OFF);
// }
//
// void onDoubleClick() {
//     ESP_LOGI(TAG, "Double click — fast blink");
//     g_led->setState(FAST_BLINK);
// }
//
// void onLongPress() {
//     ESP_LOGI(TAG, "Long press — slow blink");
//     g_led->setState(SLOW_BLINK);
// }

// =============================================================================
// APP_MAIN — ESP32 entry point (called once at boot)
// =============================================================================
extern "C" void app_main(void) {
    ESP_LOGI(TAG, "══════════════════════════════════════");
    ESP_LOGI(TAG, " Application starting (task-based)");
    ESP_LOGI(TAG, " FreeRTOS kernel running @ %d Hz", configTICK_RATE_HZ);
    ESP_LOGI(TAG, "══════════════════════════════════════");

    // =========================================================================
    // 1. CREATE DRIVER OBJECTS
    // =========================================================================
    // TODO: Instantiate your driver objects here.
    //       Use 'new' for objects that the task and callbacks need to access
    //       after app_main() continues.
    //
    // g_led    = new CLed(PIN_LED);
    // g_button = new CButton(PIN_BUTTON);
    // dht22    = new CDHT22(PIN_DHT22);
    // ntc      = new CVMA320(PIN_NTC);
    //
    // g_led->setState(OFF);  // initial state

    // =========================================================================
    // 2. ATTACH CALLBACKS
    // =========================================================================
    // TODO: Wire your callback functions to input events.
    //
    // g_button->attachSingleClick(onSingleClick);
    // g_button->attachDoubleClick(onDoubleClick);
    // g_button->attachLongPress(onLongPress);

    // =========================================================================
    // 3. CREATE FreeRTOS TASK
    // =========================================================================
    // TODO: Adjust stack size and priority based on your needs.
    //       Typical stack sizes: 2048–8192 words (each word = 4 bytes on ESP32).
    //       Priority: 0 (idle) to configMAX_PRIORITIES-1.
    //       Higher number = higher priority.

    TaskParams* params = new TaskParams();
    params->periodMs = 20;   // 20 ms = 50 Hz tick rate
    // params->led    = g_led;
    // params->button = g_button;
    // params->dht22  = dht22;
    // params->ntc    = ntc;

    TaskHandle_t taskHandle = nullptr;
    BaseType_t ret = xTaskCreate(
        worker_task,        // Task function
        "worker",           // Name (for debugging)
        4096,               // Stack size in words (4096 × 4 = 16 KB)
        (void*)params,      // Parameter passed to task
        1,                  // Priority
        &taskHandle         // Task handle (optional, can be NULL)
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create worker task!");
        return;
    }
    ESP_LOGI(TAG, "Worker task created (handle=%p)", (void*)taskHandle);

    // =========================================================================
    // 4. MAIN LOOP (optional)
    // =========================================================================
    // The worker task is doing the work.  app_main's loop can:
    //   - Go to sleep (vTaskDelay forever) — most common.
    //   - Monitor system health, toggle a heartbeat LED, etc.
    //   - Handle WiFi/BLE events if networking is added later.

    ESP_LOGI(TAG, "Entering main loop (idle)");
    while (1) {
        // TODO: Optionally blink a heartbeat LED, check for OTA, etc.
        // gpio_set_level(PIN_LED, 1);
        // vTaskDelay(pdMS_TO_TICKS(100));
        // gpio_set_level(PIN_LED, 0);
        vTaskDelay(pdMS_TO_TICKS(10000));   // sleep 10 s
    }

    // (Never reached)
}
