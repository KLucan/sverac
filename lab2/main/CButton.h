// CButton.h
#include "driver/gpio.h"

#ifndef _CButton_h
#define _CButton_h

enum ButtonState{IDLE, PRESSED, JUST_RELEASED, LONGPRESSED};

#define BUTTON_DELAY_DOUBLECLICK 150000L // ~150ms
#define BUTTON_DELAY_LONGPRESS 1000000L // ~1s

// Pointer to event handling methods
extern "C" {
    typedef void (*ButtonEventHandler)(void);
}
// void my_singeClick_function(){}

class CButton{
    public:
        CButton(int port);
        void attachSingleClick(ButtonEventHandler method){singleClick = method;};
        void attachDoubleClick(ButtonEventHandler method){doubleClick = method;};
        void attachLongPress(ButtonEventHandler method){longPress = method;};

        void tick();

    private:
        gpio_num_t m_pinNumber;
        ButtonState m_state = IDLE;
        const char *LogName = "CButton";
        int64_t m_lastPulse = 0;
        ButtonEventHandler singleClick = NULL;
        ButtonEventHandler doubleClick = NULL;
        ButtonEventHandler longPress = NULL;
};


#endif