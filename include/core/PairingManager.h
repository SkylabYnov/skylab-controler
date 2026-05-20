#pragma once

#include <atomic>

namespace Aerisys::Controller
{

class EspNowLink;

// PairingManager owns the controller-side bonding state machine.
//
// Lifecycle:
//   - At boot, if EspNowLink has a previously bonded peer  -> PAIRED.
//   - Otherwise (or after forgetPeer())                    -> PAIRING.
//   - In PAIRING, the link listens on broadcast for a packet whose
//     magic matches REQ_MAGIC. When received, the source MAC becomes
//     the bonded peer, a RESP_MAGIC confirmation is sent back, and
//     the state transitions to PAIRED.
//
// State queries are thread-safe (atomic).
class PairingManager
{
public:
    enum class State { Pairing, Paired };

    explicit PairingManager(EspNowLink *link);

    // Wire the onPairingPacket callback on the link and pick the initial
    // state from `link->hasBondedPeer()`. Must be called once at boot.
    void init();

    // Drop the current peer and re-enter PAIRING immediately.
    void forgetPeer();

    State state() const { return currentState.load(); }
    bool  isPairing() const { return state() == State::Pairing; }
    bool  isPaired()  const { return state() == State::Paired; }

private:
    void enterPairingMode();
    void enterPairedMode();

    EspNowLink         *link;
    std::atomic<State>  currentState{State::Pairing};
};

} // namespace Aerisys::Controller
