#pragma once

#include <vector>

#include "input/Button.h"

namespace Aerisys::Controller
{

// ButtonsManager polls a user-supplied list of buttons at a fixed cadence
// and dispatches press / release / long-press callbacks.
//
// Polling-only (no ISRs) keeps the firmware simple and supports an
// arbitrary number of buttons without per-instance ISR plumbing.
//
// Glitch tolerance:
//   - Mechanical bounces and Wi-Fi PA-induced GPIO glitches are filtered
//     by a temporal debounce inside task(): a new value is only accepted
//     when the raw GPIO has held it for >= Timings::BUTTON_DEBOUNCE_US.
//     This makes an external RC filter (e.g. 100 nF to GND on the input)
//     optional — useful only for very noisy wiring (long unshielded
//     leads near the ESC harness).
//   - A warmup window (Timings::BUTTON_WARMUP_US) silently syncs to the
//     resting state at boot so a button held at power-on doesn't fire a
//     phantom press on the first stable sample.
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
        // Debounced ("stable") state — only flipped when the raw sample
        // has held its new value for at least Timings::BUTTON_DEBOUNCE_US.
        bool    prevPressed     = false;
        int64_t pressStartUs    = 0;
        bool    longPressFired  = false;

        // Raw debounce tracking — last raw sample seen, and the time at
        // which it last changed value. A transition into `prevPressed`
        // only happens when (now - lastRawChangeUs) >= debounce window.
        bool    lastRaw         = false;
        int64_t lastRawChangeUs = 0;
    };

    std::vector<Button>  buttons;
    std::vector<Runtime> runtimes;
};

} // namespace Aerisys::Controller
