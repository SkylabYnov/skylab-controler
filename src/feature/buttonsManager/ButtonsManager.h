#ifndef Buttons_Manager_H
#define Buttons_Manager_H

#include "./feature/udpServer/UdpServer.h"
#include "driver/gpio.h"
#include <ControllerRequestDTO.h>


class ButtonsManager{
public:
    ButtonsManager(UdpServer* udpServer);
    void initButton();
    void Task();
private:
    static void IRAM_ATTR button_isr_handler_emergency(void* arg);
    static void IRAM_ATTR button_isr_handler_motor(void* arg);
    volatile bool buttonPressedEmergencyStop = false; 
    volatile bool buttonPressedMotorState = false; 
    gpio_num_t pinButtonEmergencyStop = GPIO_NUM_14;
    gpio_num_t pinButtonMotorState = GPIO_NUM_12;
    UdpServer* udpServer;
};


#endif // Buttons_Manager_H