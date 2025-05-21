#ifndef Buttons_Manager_H
#define Buttons_Manager_H

#include "EspNowHandler.h"
#include "driver/gpio.h"
#include <ControllerRequestDTO.h>

class ButtonsManager
{
public:
    ButtonsManager(EspNowHandler *espNowHandler);
    void initButton();
    void Task();

private:
    static void IRAM_ATTR button_isr_handler_arming(void *arg);
    static void IRAM_ATTR button_isr_handler_motor(void *arg);
    volatile bool buttonPressedMotorArming = false;
    volatile bool buttonPressedMotorState = false;

    bool buttonPressedMotorArmingValue = false;
    bool buttonPressedMotorStateValue = false;

    gpio_num_t pinButtonArming = GPIO_NUM_14;
    gpio_num_t pinButtonMotorState = GPIO_NUM_12;
    EspNowHandler *espNowHandler;
};

#endif // Buttons_Manager_H
