#pragma once

// Operational limits that shape the controller behaviour but are not
// physical (timings) nor topological (pins). Mostly safety / training
// gates on top of the joystick output.
namespace Aerisys::Controller::Limits
{
    // Throttle output multiplier. Range:
    //   -1.0f  -> no limiting (full stick range forwarded)
    //   0..1   -> applied as `throttle *= JOYSTICK_THROTTLE_LIMIT`
    // Useful as a beginner / bench training gate.
    static constexpr float JOYSTICK_THROTTLE_LIMIT = -1.0f;
}
