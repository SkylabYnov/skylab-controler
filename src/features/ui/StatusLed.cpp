#include "ui/StatusLed.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

namespace
{
    constexpr char TAG[] = "StatusLed";

    constexpr int64_t SLOW_BLINK_PERIOD_US = 500'000;
    constexpr int64_t FAST_BLINK_PERIOD_US = 200'000;
    constexpr int     TICK_PERIOD_MS       = 50;
}

namespace Aerisys::Controller
{

StatusLed::StatusLed(gpio_num_t pin)
    : pin(pin)
{
}

void StatusLed::init()
{
    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    applyLevel(false);
}

void StatusLed::start()
{
    xTaskCreate(&StatusLed::taskTrampoline,
                "statusLedTask",
                2048,
                this,
                1,
                nullptr);
}

void StatusLed::taskTrampoline(void *arg)
{
    static_cast<StatusLed*>(arg)->task();
}

void StatusLed::task()
{
    int64_t lastToggleUs = esp_timer_get_time();
    ESP_LOGI(TAG, "StatusLed task running");

    while (true) {
        const Pattern p     = currentPattern.load();
        const int64_t nowUs = esp_timer_get_time();

        switch (p) {
            case Pattern::Off:
                if (currentLevel) applyLevel(false);
                break;

            case Pattern::SolidOn:
                if (!currentLevel) applyLevel(true);
                break;

            case Pattern::SlowBlink:
                if (nowUs - lastToggleUs >= SLOW_BLINK_PERIOD_US) {
                    applyLevel(!currentLevel);
                    lastToggleUs = nowUs;
                }
                break;

            case Pattern::FastBlink:
                if (nowUs - lastToggleUs >= FAST_BLINK_PERIOD_US) {
                    applyLevel(!currentLevel);
                    lastToggleUs = nowUs;
                }
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(TICK_PERIOD_MS));
    }
}

void StatusLed::applyLevel(bool level)
{
    currentLevel = level;
    gpio_set_level(pin, level ? 1 : 0);
}

} // namespace Aerisys::Controller
