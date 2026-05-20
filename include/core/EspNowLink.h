#pragma once

#include <cstdint>
#include <functional>

#include <esp_now.h>
#include <ControllerRequestDTO.h>
#include <PingRequestDTO.h>
#include <PairingPacket.h>
#include <mpuDTO.h>

namespace Aerisys::Controller
{

// EspNowLink owns the ESP-NOW transport: Wi-Fi setup, peer table,
// persistence of the bonded peer MAC, and typed send/receive of the
// packet types we exchange with the drone.
//
// It deliberately knows nothing about pairing logic, button presses,
// or LED behaviour — higher-level modules subscribe to the typed
// callbacks below to plug their own semantics on top.
class EspNowLink
{
public:
    // Receive callbacks. Set what you care about; the rest are ignored.
    // All callbacks fire from the ESP-NOW RX context (short, no blocking).
    std::function<void(const ControllerRequestData&)> onControllerData;
    std::function<void(const PingRequestDTO&)>        onPing;
    std::function<void(const mpuDTO&)>                onMpu;
    std::function<void(const PairingPacket&,
                       const uint8_t srcMac[6])>      onPairingPacket;

    EspNowLink();
    ~EspNowLink();

    // Initialise NVS / Wi-Fi / ESP-NOW and try to restore a previously
    // bonded peer. Returns false on hard transport error.
    bool init();

    // Send the broadcast peer (FF:FF:FF:FF:FF:FF) so a pairing flow can
    // listen for packets coming from any source. Idempotent.
    bool addBroadcastPeer();

    // Add `mac` to the peer table and persist it as the bonded peer.
    bool bondPeer(const uint8_t mac[6]);

    // Drop the bonded peer (clear NVS + remove from peer table).
    void forgetPeer();

    // Typed sends. All return ESP_OK / a logged error code.
    esp_err_t sendControllerRequest(const ControllerRequestDTO &dto);
    esp_err_t sendPing();
    esp_err_t sendPairingResponse();
    esp_err_t sendToBroadcast(const void *data, size_t len);

    // Accessors.
    bool        hasBondedPeer() const;
    const uint8_t* bondedPeer() const { return peerMac; }

private:
    // C trampolines required by the esp_now API.
    static void onRecvTrampoline(const esp_now_recv_info_t *info,
                                 const uint8_t *data, int len);
    static void onSentTrampoline(const uint8_t *macAddr,
                                 esp_now_send_status_t status);
    void dispatchRecv(const esp_now_recv_info_t *info,
                      const uint8_t *data, int len);

    bool loadPeerMacFromNvs();
    bool savePeerMacToNvs();
    bool erasePeerMacFromNvs();

    static EspNowLink *instance;

    uint8_t peerMac[6]{};
    bool    peerBonded = false;
};

} // namespace Aerisys::Controller
