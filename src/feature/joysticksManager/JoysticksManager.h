#ifndef Joysticks_Manager_H
#define Joysticks_Manager_H

#include "driver/adc.h"
#include "./feature/udpServer/UdpServer.h"
#include "./core/JoystickModel/JoystickModel.h"
#include <core/ControllerRequestDTO/ControllerRequestDTO.h>

class JoysticksManager {
public:
    JoysticksManager(UdpServer* udpServer);
    void Task();
    void initJoystick();
    

private:
    adc1_channel_t pinJoystickX = ADC1_CHANNEL_6;
    adc1_channel_t pinJoystickY = ADC1_CHANNEL_7;
    UdpServer* udpServer;
    static const char *Tag;
    ControllerRequestDTO lastController; 
};

#endif // Joysticks_Manager_H
