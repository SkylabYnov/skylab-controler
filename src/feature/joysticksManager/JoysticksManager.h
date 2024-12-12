#ifndef GPIO_MANAGER_H
#define GPIO_MANAGER_H

#include "driver/adc.h"
#include "./feature/udpServer/UdpServer.h"
#include "./core/JoystickModel/JoystickModel.h"

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
    JoystickModel* oldJoystickGauche; 
};

#endif // GPIO_MANAGER_H
