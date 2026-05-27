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

        // Surface invalid pins (e.g. flash-reserved GPIO 6-11, output-only,
        // etc.) instead of letting them silently fail and the button
        // appear dead at runtime.
        esp_err_t err = gpio_config(&cfg);
        if (err != ESP_OK) {
            ESP_LOGE(TAG,
                     "gpio_config failed for '%s' on GPIO %d: %s",
                     btn.name ? btn.name : "?",
                     static_cast<int>(btn.pin),
                     esp_err_to_name(err));
            continue;
        }

        ESP_LOGI(TAG, "Registered button '%s' on GPIO %d (pull-%s)",
                 btn.name ? btn.name : "?",
                 static_cast<int>(btn.pin),
                 btn.pullUp ? "up" : "down");
    }
}

void ButtonsManager::task()
{
    ESP_LOGI(TAG, "ButtonsManager task running (%zu buttons)", buttons.size());

    // Capture the task start time so we can ignore edges during the
    // BUTTON_WARMUP_US window. During warmup we still feed the debounce
    // tracker (lastRaw / lastRawChangeUs) and prevPressed, so when the
    // gate opens the manager already knows the current resting state
    // and only fires on a real, post-warmup transition.
    const int64_t taskStartUs = esp_timer_get_time();

    while (true) {
        const int64_t nowUs    = esp_timer_get_time();
        const bool    inWarmup =
            (nowUs - taskStartUs) <
            static_cast<int64_t>(Timings::BUTTON_WARMUP_US);

        for (size_t i = 0; i < buttons.size(); ++i) {
            const Button &btn = buttons[i];
            Runtime     &rt   = runtimes[i];

            // ---- Read raw level + apply temporal debounce ----
            // A new "pressed" state is only accepted once the raw input
            // has been stable for >= BUTTON_DEBOUNCE_US. This filters
            // mechanical bounces and Wi-Fi PA-induced glitches without
            // delaying clean transitions more than one debounce window.
            const int  level = gpio_get_level(btn.pin);
            const bool raw   = btn.pullUp ? (level == 0) : (level == 1);

            if (raw != rt.lastRaw) {
                rt.lastRaw         = raw;
                rt.lastRawChangeUs = nowUs;
            }

            const bool stable =
                (nowUs - rt.lastRawChangeUs) >=
                static_cast<int64_t>(Timings::BUTTON_DEBOUNCE_US);

            // If the raw hasn't been stable long enough, keep the
            // previously-accepted state — no edge fired this cycle.
            const bool pressed = stable ? raw : rt.prevPressed;

            // During warmup: silently sync prevPressed to the current
            // stable level so that when the gate opens we don't see a
            // phantom rising/falling edge against the default-init
            // prevPressed = false. No callbacks fire in this branch.
            if (inWarmup) {
                rt.prevPressed = pressed;
                continue;
            }

            // A button has the "short-tap vs long-press" semantic only
            // when BOTH longPressMs and onLongPress are set. Otherwise
            // we keep the simple "onPressed on rising edge" behaviour.
            const bool hasLongPress = (btn.longPressMs > 0) && btn.onLongPress;

            // ---- Rising edge: press starts ----
            if (pressed && !rt.prevPressed) {
                rt.pressStartUs   = nowUs;
                rt.longPressFired = false;
                ESP_LOGI(TAG, "Pressed '%s'",
                         btn.name ? btn.name : "?");

                // Without a long-press configured, fire onPressed now
                // (historical behaviour). With a long-press configured,
                // onPressed is deferred to the release edge so it only
                // fires on a true short tap.
                if (!hasLongPress && btn.onPressed) btn.onPressed();
            }

            // ---- Long-press detection while held ----
            if (pressed && !rt.longPressFired && hasLongPress
                && (nowUs - rt.pressStartUs) >=
                       static_cast<int64_t>(btn.longPressMs) * 1000LL) {
                rt.longPressFired = true;
                ESP_LOGI(TAG, "Long press on '%s' (%d ms)",
                         btn.name ? btn.name : "?", btn.longPressMs);
                btn.onLongPress();
            }

            // ---- Falling edge: release ----
            if (!pressed && rt.prevPressed) {
                if (!rt.longPressFired) {
                    // Released before the long-press threshold:
                    //   - if the button has a long-press configured,
                    //     this is a "short tap" → fire onPressed now;
                    //   - then fire onReleased as usual.
                    if (hasLongPress && btn.onPressed) btn.onPressed();
                    if (btn.onReleased)                btn.onReleased();
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
