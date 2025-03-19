#ifndef Joysticks_Manager_H
#define Joysticks_Manager_H

#include <JoystickModel.h>
#include <ControllerRequestDTO.h>

#include "driver/adc.h"
#include "./feature/udpServer/UdpServer.h"

#define NBR_INCR_JOKTICK 10
#define TIME_MS_BETWEEN 10

class JoysticksManager {
public:
    JoysticksManager(UdpServer* udpServer);
    void Task();
    void initJoystick();
    

private:
    adc1_channel_t pinJoystickLeftX = ADC1_CHANNEL_6;
    adc1_channel_t pinJoystickLeftY = ADC1_CHANNEL_7;
    adc1_channel_t pinJoystickRightX = ADC1_CHANNEL_4;
    adc1_channel_t pinJoystickRightY = ADC1_CHANNEL_5;
    UdpServer* udpServer;
    static const char *Tag;
    JoystickModel lastJoystickModelLeft;
    JoystickModel lastJoystickModelRight; 

    JoystickModel lastJoystickModelLeftTable[NBR_INCR_JOKTICK] = {};
    JoystickModel lastJoystickModelRightTable[NBR_INCR_JOKTICK] = {};

    void addToLastRequests(JoystickModel* list, int size, const JoystickModel& newRequest);
    JoystickModel calculateAverageDTO(const JoystickModel* list, int size);
};

#endif // Joysticks_Manager_H
