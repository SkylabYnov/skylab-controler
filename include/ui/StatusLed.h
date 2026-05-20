#pragma once

#include <atomic>
#include <functional>

#include "driver/gpio.h"

namespace Aerisys::Controller
{

// StatusLed drives a single status LED through a small set of named
// patterns. The owning module flips `setPattern(...)` based on whatever
// it observes (pairing state, link health, fault flags, ...).
//
// The class runs its own FreeRTOS task on `start()` to keep blink
// timing independent of any other loop.
class StatusLed
{
public:
    enum class Pattern {
        Off,
        SolidOn,
        SlowBlink,   // ~1 Hz   (toggle every ~500 ms)
        FastBlink,   // ~2.5 Hz (toggle every ~200 ms)
    };

    explicit StatusLed(gpio_num_t pin);

    void init();
    void start();   // spawns the LED FreeRTOS task

    void setPattern(Pattern p) { currentPattern.store(p); }
    Pattern pattern() const     { return currentPattern.load(); }

private:
    static void taskTrampoline(void *arg);
    void task();
    void applyLevel(bool level);

    gpio_num_t           pin;
    std::atomic<Pattern> currentPattern{Pattern::Off};
    bool                 currentLevel = false;
};

} // namespace Aerisys::Controller
