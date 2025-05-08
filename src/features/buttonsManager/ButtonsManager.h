#ifndef Buttons_Manager_H
#define Buttons_Manager_H

#include <features/espNowHandler/EspNowHandler.h>
#include <ControllerRequestDTO.h>

#include "driver/gpio.h"


class ButtonsManager{
public:
    ButtonsManager(EspNowHandler* espNowHandler);
    void initButton();
    void Task();
private:
    static void IRAM_ATTR button_isr_handler_emergency(void* arg);
    static void IRAM_ATTR button_isr_handler_motor(void* arg);
    volatile bool buttonPressedEmergencyStop = false; 
    volatile bool buttonPressedMotorState = false; 
    gpio_num_t pinButtonEmergencyStop = GPIO_NUM_14;
    gpio_num_t pinButtonMotorState = GPIO_NUM_12;
    EspNowHandler* espNowHandler;
};


#endif // Buttons_Manager_H
