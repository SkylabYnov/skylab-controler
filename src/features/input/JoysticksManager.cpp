#include "input/JoysticksManager.h"

#include "core/EspNowLink.h"
#include "config/Limits.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include <ControllerRequestDTO.h>

namespace
{
    constexpr char TAG[] = "JoysticksManager";
}

namespace Aerisys::Controller
{

constexpr adc1_channel_t JoysticksManager::adcChannels[4];

JoysticksManager::JoysticksManager(EspNowLink *link)
    : link(link)
{
    adc1_config_width(ADC_WIDTH_BIT_12);
}

void JoysticksManager::init()
{
    // ADC_ATTEN_DB_11 was renamed to ADC_ATTEN_DB_12 in IDF 5.x; the
    // behaviour (full-range ~3.3V) is identical.
    for (adc1_channel_t ch : adcChannels) {
        adc1_config_channel_atten(ch, ADC_ATTEN_DB_12);
    }
    ESP_LOGI(TAG, "Joysticks initialised (ADC1 width 12, atten 12dB)");
}

void JoysticksManager::task()
{
    ESP_LOGI(TAG, "JoysticksManager task running");

    while (true) {
        // Read 4 channels into raw[]
        int raw[4];
        for (int i = 0; i < 4; ++i) {
            raw[i] = adc1_get_raw(adcChannels[i]);
        }

        const JoystickModel left (raw[0], raw[1]);
        const JoystickModel right(raw[2], raw[3]);

        pushSample(bufferLeft,  sumLeftX,  sumLeftY,  left);
        pushSample(bufferRight, sumRightX, sumRightY, right);

        const JoystickModel avgLeft  = rollingAverage(sumLeftX,  sumLeftY);
        const JoystickModel avgRight = rollingAverage(sumRightX, sumRightY);

        // Send only when the averaged value actually changed
        if (lastLeft != avgLeft || lastRight != avgRight) {
            ControllerRequestDTO dto;
            // esp-lib v1.1.0+: assignation par valeur, plus de `new` interne.
            // ConvertJoyStickToFlightController() positionne lui-même
            // `has_flightController = true`.
            dto.ConvertJoyStickToFlightController(avgLeft, avgRight);
            dto.initCounter();

            if (Limits::JOYSTICK_THROTTLE_LIMIT >= 0.0f && dto.has_flightController) {
                dto.flightController.throttle *= Limits::JOYSTICK_THROTTLE_LIMIT;
            }

            if (dto.has_flightController) {
                ESP_LOGD(TAG,
                         "Send joystick: pitch=%+2.3f roll=%+2.3f yaw=%+2.3f throttle=%+2.3f",
                         dto.flightController.pitch,
                         dto.flightController.roll,
                         dto.flightController.yaw,
                         dto.flightController.throttle);
            }

            link->sendControllerRequest(dto);
            lastLeft  = avgLeft;
            lastRight = avgRight;
        }

        vTaskDelay(pdMS_TO_TICKS(Timings::JOYSTICK_POLL_PERIOD_MS));
    }
}

void JoysticksManager::pushSample(JoystickModel *buf,
                                  int &sumX, int &sumY,
                                  const JoystickModel &sample)
{
    sumX -= buf[idx].x;
    sumY -= buf[idx].y;

    buf[idx] = sample;
    sumX += sample.x;
    sumY += sample.y;

    idx = (idx + 1) % AVG_WINDOW;
}

JoystickModel JoysticksManager::rollingAverage(int sumX, int sumY) const
{
    return { sumX / AVG_WINDOW, sumY / AVG_WINDOW };
}

} // namespace Aerisys::Controller
