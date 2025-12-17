#ifndef Joysticks_Manager_H
#define Joysticks_Manager_H

#include <JoystickModel.h>
#include "EspNowHandler.h"
#include <ControllerRequestDTO.h>
#include "driver/adc.h"

// #define NBR_INCR_JOYSTICK 4
// #define TIME_MS_BETWEEN 50

#define NBR_INCR_JOYSTICK 10
#define TIME_MS_BETWEEN 10

#define LimiteJoystick 0.4f //40% de la puissance si 0.4f et si -1 alors désactivé

class JoysticksManager
{
public:
    JoysticksManager(EspNowHandler *espNowHandler);
    void Task();
    void initJoystick();

private:
    static constexpr adc1_channel_t pins[4] = {
        ADC1_CHANNEL_6, // Left X GPIO 34
        ADC1_CHANNEL_7, // Left Y GPIO 35
        ADC1_CHANNEL_4, // Right X GPIO 32
        ADC1_CHANNEL_5  // Right Y GPIO 33
    };
    EspNowHandler *espNowHandler;
    static constexpr const char *Tag = "JoysticksManager";
    JoystickModel lastLeft;
    JoystickModel lastRight;

    // Circular buffers and running sums
    JoystickModel bufferLeft[NBR_INCR_JOYSTICK]{};
    JoystickModel bufferRight[NBR_INCR_JOYSTICK]{};
    int idx = 0;
    int sumLeftX = 0, sumLeftY = 0;
    int sumRightX = 0, sumRightY = 0;

    void pushSample(JoystickModel *buf, int &sumX, int &sumY, const JoystickModel &sample);
    JoystickModel getAverage(int sumX, int sumY) const;
};

#endif // Joysticks_Manager_H
