#pragma once

#include <cstdint>

// Time-related constants for the controller firmware.
// All values are absolute (no derivations), so updating one knob never
// requires recomputing another. Units are encoded in the suffix.
namespace Aerisys::Controller::Timings
{
    // ---------------------------------------------------------------------
    // Joystick sampling
    // ---------------------------------------------------------------------
    // Period of the joystick read loop. Lower = lower latency, higher CPU.
    static constexpr int JOYSTICK_POLL_PERIOD_MS = 10;
    // Number of raw samples averaged before a value is considered stable.
    static constexpr int JOYSTICK_AVG_WINDOW     = 10;

    // ---------------------------------------------------------------------
    // Buttons
    // ---------------------------------------------------------------------
    // Minimum stable time a raw GPIO sample must hold before its new
    // value is accepted by ButtonsManager. Filters mechanical bounces
    // and Wi-Fi PA-induced glitches.
    static constexpr uint64_t BUTTON_DEBOUNCE_US    = 100'000;
    // Period of the buttons task that drains the pending events queue.
    static constexpr int      BUTTON_POLL_PERIOD_MS = 50;
    // After the buttons task starts, ignore any edge for this long.
    // Avoids spurious events from settling pull-ups / power-on transients
    // and from buttons held while powering the board.
    static constexpr int64_t  BUTTON_WARMUP_US      = 250'000;

    // ---------------------------------------------------------------------
    // Safety long-press thresholds
    // ---------------------------------------------------------------------
    // Arming / motor-state toggle long-press. Short enough to stay
    // ergonomic, long enough that a bump or accidental brush cannot
    // disarm the drone mid-flight.
    static constexpr int ARMING_LONG_PRESS_MS      = 1'500;
    static constexpr int MOTOR_STATE_LONG_PRESS_MS = 1'500;

    // ---------------------------------------------------------------------
    // Association / pairing
    // ---------------------------------------------------------------------
    // How long the association button must be held to trigger a re-pairing.
    static constexpr int64_t ASSOCIATION_LONG_PRESS_MS = 5'000;
    // Toggle period of the status LED while waiting for a pairing packet.
    static constexpr int64_t ASSOCIATION_LED_BLINK_US  = 200'000;

    // ---------------------------------------------------------------------
    // ESP-NOW link
    // ---------------------------------------------------------------------
    // Period at which the controller emits a ping to the drone.
    static constexpr int64_t PING_INTERVAL_US = 1'000'000;
    // After this much time without an incoming packet, the link is
    // considered down (used by future fail-safe).
    static constexpr int64_t LINK_TIMEOUT_US  = 500'000;
}
