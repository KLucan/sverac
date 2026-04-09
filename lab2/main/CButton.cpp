#include <stdio.h>
#include "CButton.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

CButton::CButton(int port){
    m_pinNumber = (gpio_num_t)port;
    ESP_LOGI(LogName, "Configure port [%d] to input!!!", port);
    gpio_set_pull_mode(m_pinNumber, GPIO_PULLDOWN_ONLY);
    gpio_reset_pin(m_pinNumber);
    gpio_set_direction(m_pinNumber, GPIO_MODE_INPUT);
}

void CButton::tick(){;
    switch(m_state){
        case IDLE:
            if ((esp_timer_get_time() - m_lastPulse) < BUTTON_DELAY_DOUBLECLICK) break;
            if (!gpio_get_level(m_pinNumber)){
                m_lastPulse = esp_timer_get_time();
                ESP_LOGI(LogName, "BUTTON WAKE");
                m_state = PRESSED;
            }
            break;
        case PRESSED:
            if ((esp_timer_get_time() - m_lastPulse) > BUTTON_DELAY_LONGPRESS) {
                    ESP_LOGI(LogName, "BUTTON LONGPRESS");
                    m_lastPulse = esp_timer_get_time();
                    m_state = LONGPRESSED;
                    longPress();
            }
            else if (gpio_get_level(m_pinNumber)) {
                    ESP_LOGI(LogName, "BUTTON RELEASED");
                    m_lastPulse = esp_timer_get_time();
                    m_state = JUST_RELEASED;
                }
            break;
        case JUST_RELEASED:
            if ((esp_timer_get_time() - m_lastPulse) < BUTTON_DELAY_DOUBLECLICK){
                if (!gpio_get_level(m_pinNumber)){ 
                    ESP_LOGI(LogName, "BUTTON DOUBLECLICK");
                    m_lastPulse = esp_timer_get_time();
                    m_state = IDLE;
                    doubleClick();
                }
            }
            else {
                ESP_LOGI(LogName, "BUTTON CLICK");
                m_lastPulse = esp_timer_get_time();
                m_state = IDLE;
                singleClick();
            }
            break;
        case LONGPRESSED:
            if (!gpio_get_level(m_pinNumber))
            {
                ESP_LOGI(LogName, "BUTTON STILL PRESSED");
            }
            else m_state = IDLE;
            break;
        default:
            break;
    }
}
