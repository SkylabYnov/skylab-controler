#include "JoysticksManager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

JoysticksManager::JoysticksManager(EspNowHandler *espNowHandler)
    : espNowHandler(espNowHandler)
{
    adc1_config_width(ADC_WIDTH_BIT_12);
}

void JoysticksManager::Task()
{
    while (true)
    {
        // Read ADC in a loop
        int raw[4];
        for (int i = 0; i < 4; ++i)
        {
            raw[i] = adc1_get_raw(pins[i]);
        }

        JoystickModel left(raw[0], raw[1]);
        JoystickModel right(raw[2], raw[3]);

        // Update circular buffers and sums
        pushSample(bufferLeft, sumLeftX, sumLeftY, left);
        pushSample(bufferRight, sumRightX, sumRightY, right);

        // Compute averages
        JoystickModel avgLeft = getAverage(sumLeftX, sumLeftY);
        JoystickModel avgRight = getAverage(sumRightX, sumRightY);

        // JoystickModel::operator!= has a 50-LSB tolerance, so slow stick
        // movements would never trigger a send. Force a periodic resend so
        // gradual commands always reach the drone.
        int64_t nowMs = esp_timer_get_time() / 1000;
        bool changed = (lastSentLeft != avgLeft || lastSentRight != avgRight);
        bool heartbeatDue = (nowMs - lastSentTimeMs) >= JOYSTICK_HEARTBEAT_MS;

        if (changed || heartbeatDue)
        {
            ControllerRequestDTO dto;
            dto.ConvertJoyStickToFlightController(avgLeft, avgRight);
            dto.initCounter();

            if(LimiteJoystick != -1){
                dto.flightController->throttle = dto.flightController->throttle * LimiteJoystick;
            }

            ESP_LOGI(Tag, "Envoi Joystick : (pitch=%+2.3f, roll=%+2.3f, yaw=%+2.3f, throttle=%+2.3f)",
                     dto.flightController->pitch,
                     dto.flightController->roll,
                     dto.flightController->yaw,
                     dto.flightController->throttle);

            espNowHandler->send_data(dto);
            lastSentLeft = avgLeft;
            lastSentRight = avgRight;
            lastSentTimeMs = nowMs;
        }

        vTaskDelay(pdMS_TO_TICKS(TIME_MS_BETWEEN));
    }
}

void JoysticksManager::pushSample(JoystickModel *buf, int &sumX, int &sumY, const JoystickModel &sample)
{
    // Remove oldest
    sumX -= buf[idx].x;
    sumY -= buf[idx].y;

    // Insert new
    buf[idx] = sample;
    sumX += sample.x;
    sumY += sample.y;

    // Advance index
    idx = (idx + 1) % NBR_INCR_JOYSTICK;
}

JoystickModel JoysticksManager::getAverage(int sumX, int sumY) const
{
    int avgX = sumX / NBR_INCR_JOYSTICK;
    int avgY = sumY / NBR_INCR_JOYSTICK;
    return {avgX, avgY};
}

void JoysticksManager::initJoystick()
{
    for (auto ch : pins)
    {
        adc1_config_channel_atten(ch, ADC_ATTEN_DB_11);
    }
}
