#include "core/PairingManager.h"
#include "core/EspNowLink.h"

#include <cstring>
#include <esp_log.h>
// NOTE: PairingPacket.h has no include guard upstream — pulled transitively
// via core/EspNowLink.h. Do NOT include it directly here or you will hit a
// "conflicting declaration 'typedef struct PairingPacket'" link error.

namespace
{
    constexpr char TAG[] = "PairingManager";
}

namespace Aerisys::Controller
{

PairingManager::PairingManager(EspNowLink *link)
    : link(link)
{
}

void PairingManager::init()
{
    // Hook the pairing-packet callback on the transport.
    link->onPairingPacket = [this](const PairingPacket &pkt,
                                   const uint8_t srcMac[6]) {
        if (!isPairing()) return;
        if (std::strncmp(pkt.magic, REQ_MAGIC, sizeof(pkt.magic)) != 0) return;

        ESP_LOGI(TAG, "Pairing request received from %02x:%02x:%02x:%02x:%02x:%02x",
                 srcMac[0], srcMac[1], srcMac[2],
                 srcMac[3], srcMac[4], srcMac[5]);

        if (!link->bondPeer(srcMac)) {
            ESP_LOGE(TAG, "Failed to bond peer, staying in pairing mode");
            return;
        }

        if (link->sendPairingResponse() == ESP_OK) {
            ESP_LOGI(TAG, "Pairing confirmation sent");
        } else {
            ESP_LOGW(TAG, "Pairing confirmation send error");
        }

        enterPairedMode();
    };

    if (link->hasBondedPeer()) {
        enterPairedMode();
    } else {
        ESP_LOGW(TAG, "No bonded peer, entering pairing mode");
        enterPairingMode();
    }
}

void PairingManager::forgetPeer()
{
    link->forgetPeer();
    enterPairingMode();
}

void PairingManager::enterPairingMode()
{
    currentState.store(State::Pairing);
    link->addBroadcastPeer();
    ESP_LOGI(TAG, "Pairing mode ACTIVE");
}

void PairingManager::enterPairedMode()
{
    currentState.store(State::Paired);
    ESP_LOGI(TAG, "Paired");
}

} // namespace Aerisys::Controller
