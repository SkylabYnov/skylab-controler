#pragma once

#include <cstdint>
#include <cstddef>

#include <JoystickModel.h>

namespace Aerisys::Controller
{

class EspNowLink;

// Wire format coming from a host PC (Unity, Python tool, ...) over UART.
// Sticks are normalised in [-1, +1], buttons are 0 / 1.
//
// Frame on the wire:
//   0xAA 0x55 | length(1B) | payload(length B) | checksum(1B)
//
// checksum = (sum of payload bytes) & 0xFF
struct ControllerPacket
{
    float   RightStickY;
    float   RightStickX;
    float   LeftStickY;
    float   LeftStickX;
    uint8_t motorState;
    uint8_t motorArming;
};

// PcSerialBridge mirrors what the joystick + buttons modules do, but
// reads input from UART instead of physical hardware. This lets a host
// PC drive the radio link for tests, scripting, or replaying scenarios.
//
// Like JoysticksManager, sends are change-only on the sticks.
class PcSerialBridge
{
public:
    explicit PcSerialBridge(EspNowLink *link, int baudRate = 115200);
    ~PcSerialBridge();

    // Long-running task: parse frames forever, forward to the link.
    void task();

private:
    static uint8_t computeChecksum(const uint8_t *data, size_t len);
    void           dispatchPacket(const ControllerPacket &pkt);

    EspNowLink *link;

    JoystickModel lastLeft;
    JoystickModel lastRight;
    bool          lastMotorState  = false;
    bool          lastMotorArming = false;
};

} // namespace Aerisys::Controller
