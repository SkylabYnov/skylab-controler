#pragma once

#include <functional>

#include "driver/gpio.h"

namespace Aerisys::Controller
{

// Declarative description of a single physical button.
//
// Only fill the callbacks you care about — the rest are ignored.
// Press and release events are debounced inside ButtonsManager:
// transitions are only accepted once the raw GPIO has been stable for
// at least Timings::BUTTON_DEBOUNCE_US.
//
// Callback rules:
//   - If onLongPress is NOT configured (longPressMs == 0 or onLongPress
//     is empty), `onPressed` fires on the rising edge (immediate). This
//     is the historical behaviour for buttons that only do a short press.
//
//   - If onLongPress IS configured, `onPressed` is DEFERRED to the
//     release event, and is fired only when the release happens BEFORE
//     the long-press threshold (= "short tap"). A long press fires
//     `onLongPress` only; the matching release fires neither
//     `onPressed` nor `onReleased`. This avoids double-firing on
//     mutually-exclusive short/long actions.
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
