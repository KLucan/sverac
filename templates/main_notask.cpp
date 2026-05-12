// main_notask.cpp — Template: Super-loop (no extra tasks, Round-Robin)
// =============================================================================
// Use this template when:
//   - The application is simple (one or two drivers).
//   - You do NOT need parallel execution.
//   - You prefer a classic Arduino-style "super loop".
//   - You want minimal RAM overhead (no extra task stacks).
//
// PATTERN:
//   app_main() creates driver objects, then enters an infinite while(1) loop.
//   Inside the loop, each driver's tick() is called, sensors are polled,
//   and a vTaskDelay() controls the loop rate.
//   The main task IS the only task (plus FreeRTOS idle task and maybe WiFi stack).
//
// WARNING: Since everything runs in one loop, long-running operations (e.g.
//          slow sensor reads, blocking UART) will delay all other processing.
//          If you need responsiveness, use the task-based or interrupt templates.
//
// INSTRUCTIONS:
//   1. Copy this file to main/app_main.cpp.
//   2. Add your driver #includes.
//   3. Define pins and constants.
//   4. Create driver objects at the top of app_main().
//   5. Fill in the super-loop body.
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

// --- LOG TAG ----------------------------------------------------------------
static const char* TAG = "MAIN";

// --- GPIO PIN DEFINITIONS ----------------------------------------------------
// TODO: Set your GPIO pins.
#define PIN_LED      GPIO_NUM_2
#define PIN_BUTTON   GPIO_NUM_0

// --- TIMING ------------------------------------------------------------------
// TODO: Adjust main loop period (milliseconds).
#define MAIN_LOOP_PERIOD_MS     20      // 20 ms = 50 Hz

// =============================================================================
// APP_MAIN — ESP32 entry point
// =============================================================================
extern "C" void app_main(void) {
    ESP_LOGI(TAG, "══════════════════════════════════════");
    ESP_LOGI(TAG, " Application starting (super-loop)");
    ESP_LOGI(TAG, " Loop period: %d ms", MAIN_LOOP_PERIOD_MS);
    ESP_LOGI(TAG, "══════════════════════════════════════");

    // =========================================================================
    // 1. CREATE DRIVER OBJECTS (on stack — they live for the whole loop)
    // =========================================================================
    // TODO: Instantiate your driver objects.
    //
    // CLed     led(PIN_LED);
    // CButton  button(PIN_BUTTON);
    //
    // led.setState(OFF);

    // =========================================================================
    // 2. ATTACH CALLBACKS (if using button/input drivers)
    // =========================================================================
    // TODO: If you're using input drivers with callbacks, you can either:
    //   a) Attach callbacks and call input.tick() in the loop.
    //   b) Skip callbacks and just poll input.isActive() / input.getLevel().
    //
    // button.attachOnClick([]() {
    //     ESP_LOGI(TAG, "Button clicked!");
    //     // toggle LED, etc.
    // });

    // =========================================================================
    // 3. OPTIONAL INITIAL DELAYS
    // =========================================================================
    // Some devices need time to stabilise after power-on.
    // vTaskDelay(pdMS_TO_TICKS(2000));   // 2 s startup delay

    // =========================================================================
    // 4. SUPER-LOOP
    // =========================================================================
    ESP_LOGI(TAG, "Entering super-loop");

    // For precise periodic timing:
    TickType_t lastWakeTime = xTaskGetTickCount();

    while (1) {
        // --- A) Tick state-machine drivers -----------------------------------
        // TODO: Call tick() on each driver that has a state machine.
        // led.tick();
        // button.tick();

        // --- B) Poll simple sensors ------------------------------------------
        // TODO: For sensors without tick(), read directly.
        //
        // Example: read button state without callbacks:
        // if (button.isActive()) {
        //     ESP_LOGI(TAG, "Button pressed");
        //     led.setState(ON);
        // } else {
        //     led.setState(OFF);
        // }

        // --- C) Periodic actions at lower rates -------------------------------
        // Use counters or timestamps to do things every N loop iterations.
        //
        // static int counter = 0;
        // counter++;
        // if (counter % 50 == 0) {   // every 50 loops = 1 s at 20 ms period
        //     ESP_LOGI(TAG, "1-second tick");
        //     // read a slow sensor here
        // }

        // --- D) Logging / debugging ------------------------------------------
        // static int64_t lastLog = 0;
        // int64_t now = esp_timer_get_time();
        // if ((now - lastLog) > 5000000LL) {  // every 5 seconds
        //     lastLog = now;
        //     ESP_LOGI(TAG, "Heartbeat — system running OK");
        // }

        // --- E) Delay (controls loop rate) -----------------------------------
        // Option 1: Simple fixed delay (drift accumulates)
        // vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_PERIOD_MS));

        // Option 2: Precise periodic timing (compensates for loop execution time)
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(MAIN_LOOP_PERIOD_MS));
    }

    // (Never reached)
}
