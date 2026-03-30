/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
//#include "led_strip.h"
#include "sdkconfig.h"
#include "CLed.h"
#include "CButton.h"

static const char *TAG = "MAIN";

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO 2 //CONFIG_BLINK_GPIO
#define BUTTON_GPIO 0 //0 is boot button

struct task_params
{
    CLed *led;
    CButton *btn;
};


//
//Task Loop
//
void task_loop (void *parameters)
{
    ESP_LOGI(TAG, "Start TASK Loop.");
    
    CLed *led;
    CButton *btn;
    task_params* params = (task_params*)parameters;
    led = params->led;
    ESP_LOGI(TAG, "Get Led pointer.");
    btn = params->btn;
    ESP_LOGI(TAG, "Get Btn pointer.");
    
    while(1) {
        //Do tick
        led->tick();
        btn->tick();
        vTaskDelay(20 / portTICK_PERIOD_MS);      
    }
}

CLed *led1 = NULL;
TaskHandle_t xHandle = NULL;

void onClickLed(){
    LedStatus state = led1->getLedState();
    if (state != LedStatus::BLINK) led1->setLedState(LedStatus::BLINK);
    else led1->setLedState(LedStatus::OFF);
}

void onDoubleClickLed(){
    LedStatus state = led1->getLedState();
    if (state != LedStatus::FAST_BLINK) led1->setLedState(LedStatus::FAST_BLINK);
}

void onLongPressLed(){
    LedStatus state = led1->getLedState();
    if (state != LedStatus::SLOW_BLINK) led1->setLedState(LedStatus::SLOW_BLINK);
}

//ESP32 mian function
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Start MAIN.");
    
    //Create CLed object
    led1 = new CLed(BLINK_GPIO);

    led1->setLedState(LedStatus::OFF);

    //Create CButton object
    CButton *btn1 = new CButton(BUTTON_GPIO);

    btn1->attachSingleClick(onClickLed);
    btn1->attachDoubleClick(onDoubleClickLed);
    btn1->attachLongPress(onLongPressLed);

    //Create Task
    ESP_LOGI(TAG, "btn: %d", btn1);
    task_params *params = new task_params{led1, btn1};  
    ESP_LOGI(TAG, "Start Task Create.");
    xTaskCreate(task_loop,      //Task function
                "ledLoop",      //Name of task in task scheduler
                1024*10,         //Stack size
                (void*)params,   //Parameter send to function
                1,              //Priority
                &xHandle);      //task handler 
    ESP_LOGI(TAG, "Task Created."); 
    
    //Main loop
    while(1) {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        continue;
        led1->setLedState(LedStatus::BLINK);
        vTaskDelay(10000 / portTICK_PERIOD_MS);

        led1->setLedState(LedStatus::FAST_BLINK);
        vTaskDelay(10000 / portTICK_PERIOD_MS);

        led1->setLedState(LedStatus::SLOW_BLINK);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }

}
