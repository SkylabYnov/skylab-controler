#pragma once

#include <vector>

#include "input/Button.h"

namespace Aerisys::Controller
{

// ButtonsManager polls a user-supplied list of buttons at a fixed cadence
// and dispatches press / release / long-press callbacks.
//
// Polling-only (no ISRs) keeps the firmware simple, dodges debounce
// edge cases entirely (a press only counts when the GPIO is stable for
// a full poll period), and supports an arbitrary number of buttons
// without per-instance ISR plumbing.
//
// Adding a button is one entry in the constructor's vector.
class ButtonsManager
{
public:
    explicit ButtonsManager(std::vector<Button> buttons);

    // Configure pin modes from the Button list. Must be called before task().
    void init();

    // Long-running task: poll levels, edge-detect, fire callbacks.
    // Spawn with xTaskCreate(...).
    void task();

private:
    struct Runtime {
        bool    prevPressed     = false;
        int64_t pressStartUs    = 0;
        bool    longPressFired  = false;
    };

    std::vector<Button>  buttons;
    std::vector<Runtime> runtimes;
};

} // namespace Aerisys::Controller
