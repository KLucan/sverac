// main_interrupt.cpp — Template: GPIO interrupt-driven main program
// =============================================================================
// Use this template when you need:
//   - Immediate response to GPIO level changes (e.g. button press, PIR sensor).
//   - Low-latency event handling without polling.
//   - Wake-from-sleep on external trigger.
//
// PATTERN:
//   app_main() installs the GPIO ISR service, then registers per-pin ISR
//   handlers.  The ISRs do minimal work (set a flag, send to queue) and
//   a FreeRTOS task processes the events.  This keeps ISRs short.
//
//   Option A (simple):  Use gpio_isr_register() for a single global ISR.
//   Option B (scalable): Use gpio_install_isr_service() + gpio_isr_handler_add()
//                         for per-pin handlers routed through a task/queue.
//
//   This template uses Option B with a FreeRTOS queue for ISR → Task communication.
//
// ESP-IDF v5.4 GPIO interrupt API:
//   gpio_install_isr_service(), gpio_isr_handler_add(),
//   gpio_set_intr_type(), gpio_intr_enable()
//
// INSTRUCTIONS:
//   1. Copy this file to main/app_main.cpp.
//   2. Add your driver #includes (output drivers only; input is handled by ISR).
//   3. Define the interrupt GPIO pins and edge types.
//   4. Implement the ISR handlers (IRAM_ATTR, keep them SHORT).
//   5. Implement the event processing task.
// =============================================================================

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "sdkconfig.h"

// --- DRIVER HEADERS ----------------------------------------------------------
// TODO: Include your output/communication driver headers.
// #include "CLed.h"

// --- LOG TAG ----------------------------------------------------------------
static const char* TAG = "MAIN";

// --- GPIO PIN DEFINITIONS ----------------------------------------------------
// TODO: Define pins that will trigger interrupts.
#define PIN_INTERRUPT_0    GPIO_NUM_0     // BOOT button
#define PIN_INTERRUPT_1    GPIO_NUM_4     // Example: second button or sensor

// --- INTERRUPT EDGE TYPE -----------------------------------------------------
// GPIO_INTR_POSEDGE   — rising edge  (0 → 1)
// GPIO_INTR_NEGEDGE   — falling edge (1 → 0)
// GPIO_INTR_ANYEDGE   — both edges
// GPIO_INTR_LOW_LEVEL — low level (use with caution — can retrigger)
// GPIO_INTR_HIGH_LEVEL— high level
#define PIN_INTR_TYPE_0    GPIO_INTR_NEGEDGE   // button press (active low)
#define PIN_INTR_TYPE_1    GPIO_INTR_POSEDGE

// --- DEBOUNCE (simple software debounce in ISR) ------------------------------
// ISRs should be fast, so debouncing via time check is best effort.
// For robust debouncing, use the input-driver template instead.
#define DEBOUNCE_US         50000   // 50 ms

// =============================================================================
// EVENT STRUCT — Passed from ISR to task via queue
// =============================================================================
// TODO: Add fields if you need to pass more data from the ISR.
struct InterruptEvent {
    gpio_num_t  pin;            // Which pin triggered
    int         level;          // Current pin level (snapshot)
    int64_t     timestampUs;    // When it happened (esp_timer_get_time)
};

// =============================================================================
// QUEUE — Communicates ISR events to the task
// =============================================================================
static QueueHandle_t g_eventQueue = nullptr;
#define EVENT_QUEUE_LENGTH  16

// =============================================================================
// ISR HANDLERS — Keep these SHORT (no logging, no delays, no blocking calls)
// =============================================================================
// The IRAM_ATTR places the function in IRAM so it works even during flash ops.
// Send a simple event to the queue and let the task handle the complex logic.

static int64_t g_lastIsrTime0 = 0;   // for simple debounce
static int64_t g_lastIsrTime1 = 0;

// --- ISR for PIN_INTERRUPT_0 ---
static void IRAM_ATTR isr_handler_0(void* arg) {
    // Simple debounce: ignore if too soon after last trigger
    int64_t now = esp_timer_get_time();
    if ((now - g_lastIsrTime0) < DEBOUNCE_US) {
        return;
    }
    g_lastIsrTime0 = now;

    InterruptEvent evt = {};
    evt.pin         = PIN_INTERRUPT_0;
    evt.level       = gpio_get_level(PIN_INTERRUPT_0);
    evt.timestampUs = now;

    // Send to queue (non-blocking from ISR)
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(g_eventQueue, &evt, &higherPriorityTaskWoken);

    // If sending woke a higher-priority task, yield at ISR end
    if (higherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

// --- ISR for PIN_INTERRUPT_1 ---
static void IRAM_ATTR isr_handler_1(void* arg) {
    int64_t now = esp_timer_get_time();
    if ((now - g_lastIsrTime1) < DEBOUNCE_US) {
        return;
    }
    g_lastIsrTime1 = now;

    InterruptEvent evt = {};
    evt.pin         = PIN_INTERRUPT_1;
    evt.level       = gpio_get_level(PIN_INTERRUPT_1);
    evt.timestampUs = now;

    BaseType_t higherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(g_eventQueue, &evt, &higherPriorityTaskWoken);

    if (higherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

// =============================================================================
// EVENT PROCESSING TASK — Handles events from the queue
// =============================================================================
// This is a normal FreeRTOS task, so it can log, delay, call driver methods.
// TODO: Customize the event handling logic.
void event_task(void* parameters) {
    ESP_LOGI(TAG, "Event task started");

    // TODO: Access your driver objects here (pass via params or global).
    // CLed* led = (CLed*)parameters;

    InterruptEvent evt;

    while (1) {
        // Block until an event arrives
        if (xQueueReceive(g_eventQueue, &evt, portMAX_DELAY) == pdTRUE) {

            ESP_LOGI(TAG, "ISR event: pin=GPIO%d level=%d time=%lld µs",
                     (int)evt.pin, evt.level, evt.timestampUs);

            // --- Handle event based on which pin triggered ---
            switch (evt.pin) {

                case PIN_INTERRUPT_0:
                    // TODO: Your logic for button 0 press.
                    // Example: toggle LED state
                    // LedStatus state = led->getState();
                    // led->setState((state != BLINK) ? BLINK : OFF);
                    ESP_LOGI(TAG, "Button 0 pressed!");
                    break;

                case PIN_INTERRUPT_1:
                    // TODO: Your logic for pin 1 event.
                    ESP_LOGI(TAG, "Interrupt on GPIO%d!", (int)evt.pin);
                    break;

                default:
                    ESP_LOGW(TAG, "Unknown interrupt pin: %d", (int)evt.pin);
                    break;
            }

            // --- Optional: Count events for double-click detection, etc. ---
            // You can implement multi-click logic here using timestamps.
            // static int64_t lastClickTime = 0;
            // static int clickCount = 0;
            // ...
        }
    }
}

// =============================================================================
// APP_MAIN — Set up interrupts and start event task
// =============================================================================
extern "C" void app_main(void) {
    ESP_LOGI(TAG, "══════════════════════════════════════");
    ESP_LOGI(TAG, " Application starting (interrupt-driven)");
    ESP_LOGI(TAG, "══════════════════════════════════════");

    // =========================================================================
    // 1. CREATE DRIVER OBJECTS (output / comms drivers)
    // =========================================================================
    // TODO: Instantiate output drivers that the ISR task will control.
    // CLed* led = new CLed(PIN_LED);
    // led->setState(OFF);

    // =========================================================================
    // 2. CREATE EVENT QUEUE
    // =========================================================================
    g_eventQueue = xQueueCreate(EVENT_QUEUE_LENGTH, sizeof(InterruptEvent));
    if (g_eventQueue == nullptr) {
        ESP_LOGE(TAG, "Failed to create event queue");
        return;
    }

    // =========================================================================
    // 3. CONFIGURE GPIOs FOR INTERRUPTS
    // =========================================================================
    // --- Pin 0 ---
    gpio_reset_pin(PIN_INTERRUPT_0);
    gpio_set_direction(PIN_INTERRUPT_0, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_INTERRUPT_0, GPIO_PULLUP_ONLY);  // active low
    gpio_set_intr_type(PIN_INTERRUPT_0, PIN_INTR_TYPE_0);

    // --- Pin 1 ---
    gpio_reset_pin(PIN_INTERRUPT_1);
    gpio_set_direction(PIN_INTERRUPT_1, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_INTERRUPT_1, GPIO_PULLDOWN_ONLY); // active high
    gpio_set_intr_type(PIN_INTERRUPT_1, PIN_INTR_TYPE_1);

    // =========================================================================
    // 4. INSTALL GPIO ISR SERVICE
    // =========================================================================
    // This creates a global ISR dispatcher.  Individual handlers are added next.
    // ESP_INTR_FLAG_IRAM: ISR runs from IRAM (safe during flash ops).
    // Use ESP_INTR_FLAG_LEVEL1–3 for interrupt priority levels.
    esp_err_t ret = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_install_isr_service failed: %s", esp_err_to_name(ret));
        return;
    }

    // =========================================================================
    // 5. ADD PER-PIN ISR HANDLERS
    // =========================================================================
    ret = gpio_isr_handler_add(PIN_INTERRUPT_0, isr_handler_0, nullptr);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_isr_handler_add(pin0) failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = gpio_isr_handler_add(PIN_INTERRUPT_1, isr_handler_1, nullptr);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_isr_handler_add(pin1) failed: %s", esp_err_to_name(ret));
        return;
    }

    // =========================================================================
    // 6. ENABLE INTERRUPTS
    // =========================================================================
    gpio_intr_enable(PIN_INTERRUPT_0);
    gpio_intr_enable(PIN_INTERRUPT_1);
    ESP_LOGI(TAG, "GPIO interrupts enabled on pins %d, %d",
             (int)PIN_INTERRUPT_0, (int)PIN_INTERRUPT_1);

    // =========================================================================
    // 7. CREATE EVENT PROCESSING TASK
    // =========================================================================
    // TODO: Pass driver pointers as task parameter if needed.
    TaskHandle_t eventTaskHandle = nullptr;
    ret = (esp_err_t)xTaskCreate(
        event_task,         // Task function
        "event_task",       // Name
        4096,               // Stack size (words)
        nullptr,            // Parameter (e.g. led)
        2,                  // Priority (higher than main for responsiveness)
        &eventTaskHandle
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create event task");
        return;
    }
    ESP_LOGI(TAG, "Event task created");

    // =========================================================================
    // 8. MAIN LOOP (idle)
    // =========================================================================
    ESP_LOGI(TAG, "Entering main loop (idle — all work in event task)");
    while (1) {
        // Optionally: heartbeat LED, system monitoring, etc.
        vTaskDelay(pdMS_TO_TICKS(10000));   // 10 s
    }
}
