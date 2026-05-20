#include "input/ButtonsManager.h"

#include "config/Timings.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

namespace
{
    constexpr char TAG[] = "ButtonsManager";
}

namespace Aerisys::Controller
{

ButtonsManager::ButtonsManager(std::vector<Button> buttons)
    : buttons(std::move(buttons))
{
    runtimes.resize(this->buttons.size());
}

void ButtonsManager::init()
{
    for (const auto &btn : buttons) {
        gpio_config_t cfg = {};
        cfg.pin_bit_mask  = 1ULL << btn.pin;
        cfg.mode          = GPIO_MODE_INPUT;
        cfg.pull_up_en    = btn.pullUp ? GPIO_PULLUP_ENABLE  : GPIO_PULLUP_DISABLE;
        cfg.pull_down_en  = btn.pullUp ? GPIO_PULLDOWN_DISABLE : GPIO_PULLDOWN_ENABLE;
        cfg.intr_type     = GPIO_INTR_DISABLE;
        gpio_config(&cfg);
        ESP_LOGI(TAG, "Registered button '%s' on GPIO %d (pull-%s)",
                 btn.name ? btn.name : "?",
                 static_cast<int>(btn.pin),
                 btn.pullUp ? "up" : "down");
    }
}

void ButtonsManager::task()
{
    ESP_LOGI(TAG, "ButtonsManager task running (%zu buttons)", buttons.size());

    while (true) {
        const int64_t nowUs = esp_timer_get_time();

        for (size_t i = 0; i < buttons.size(); ++i) {
            const Button &btn = buttons[i];
            Runtime     &rt   = runtimes[i];

            const int  level   = gpio_get_level(btn.pin);
            const bool pressed = btn.pullUp ? (level == 0) : (level == 1);

            // ---- Rising edge: press starts ----
            if (pressed && !rt.prevPressed) {
                rt.pressStartUs   = nowUs;
                rt.longPressFired = false;
                if (btn.onPressed) btn.onPressed();
            }

            // ---- Long-press detection while held ----
            if (pressed && !rt.longPressFired
                && btn.longPressMs > 0 && btn.onLongPress
                && (nowUs - rt.pressStartUs) >= (int64_t)btn.longPressMs * 1000LL) {
                rt.longPressFired = true;
                ESP_LOGI(TAG, "Long press on '%s' (%d ms)",
                         btn.name ? btn.name : "?", btn.longPressMs);
                btn.onLongPress();
            }

            // ---- Falling edge: release ----
            if (!pressed && rt.prevPressed) {
                // Only fire onReleased if no long-press consumed the event.
                if (!rt.longPressFired && btn.onReleased) {
                    btn.onReleased();
                }
                rt.pressStartUs   = 0;
                rt.longPressFired = false;
            }

            rt.prevPressed = pressed;
        }

        vTaskDelay(pdMS_TO_TICKS(Timings::BUTTON_POLL_PERIOD_MS));
    }
}

} // namespace Aerisys::Controller
