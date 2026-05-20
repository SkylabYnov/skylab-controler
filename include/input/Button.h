#pragma once

#include <functional>

#include "driver/gpio.h"

namespace Aerisys::Controller
{

// Declarative description of a single physical button.
//
// Only fill the callbacks you care about — the rest are ignored.
// Press and release events are debounced by ButtonsManager via its
// polling cadence (Timings::BUTTON_POLL_PERIOD_MS).
//
// onLongPress, when set, suppresses the corresponding onReleased call
// so the same release does not look like both a click and a long press.
struct Button
{
    gpio_num_t   pin;
    const char  *name      = nullptr;
    bool         pullUp    = true;   // internal pull-up, active-low

    // Single-press callbacks (default-init to empty so designated
    // initializers can skip them without triggering -Wmissing-field-initializers).
    std::function<void()> onPressed   = {};
    std::function<void()> onReleased  = {};

    // Long-press support (set both fields together to enable)
    int                  longPressMs = 0;
    std::function<void()> onLongPress = {};
};

} // namespace Aerisys::Controller
