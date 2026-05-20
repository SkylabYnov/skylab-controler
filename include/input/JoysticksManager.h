#pragma once

#include "config/Pins.h"
#include "config/Timings.h"

#include "driver/adc.h"
#include <JoystickModel.h>

namespace Aerisys::Controller
{

class EspNowLink;

// JoysticksManager reads four ADC channels (two two-axis joysticks),
// applies a sliding-window average, and forwards a ControllerRequestDTO
// to the radio link whenever the smoothed value changes.
//
// Update cadence is Timings::JOYSTICK_POLL_PERIOD_MS;
// the averaging window is Timings::JOYSTICK_AVG_WINDOW samples.
class JoysticksManager
{
public:
    explicit JoysticksManager(EspNowLink *link);

    void init();   // configure ADC width + attenuations
    void task();   // long-running sampling + send loop

private:
    static constexpr int AVG_WINDOW = Timings::JOYSTICK_AVG_WINDOW;

    void pushSample(JoystickModel *buf, int &sumX, int &sumY,
                    const JoystickModel &sample);
    JoystickModel rollingAverage(int sumX, int sumY) const;

    EspNowLink   *link;

    JoystickModel lastLeft;
    JoystickModel lastRight;

    JoystickModel bufferLeft[AVG_WINDOW]{};
    JoystickModel bufferRight[AVG_WINDOW]{};
    int idx       = 0;
    int sumLeftX  = 0;
    int sumLeftY  = 0;
    int sumRightX = 0;
    int sumRightY = 0;

    static constexpr adc1_channel_t adcChannels[4] = {
        Pins::JOY_LEFT_X,
        Pins::JOY_LEFT_Y,
        Pins::JOY_RIGHT_X,
        Pins::JOY_RIGHT_Y,
    };
};

} // namespace Aerisys::Controller
