#include "core/EspNowLink.h"

#include <cstring>

#include <esp_log.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include <nvs_flash.h>

namespace
{
    constexpr char       TAG[]            = "EspNowLink";
    constexpr char       NVS_NAMESPACE[]  = "storage";
    constexpr char       NVS_KEY_PEER[]   = "controller_mac";
    constexpr uint8_t    BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    bool isAllZero(const uint8_t mac[6])
    {
        for (int i = 0; i < 6; ++i) {
            if (mac[i] != 0) return false;
        }
        return true;
    }
} // namespace

namespace Aerisys::Controller
{

EspNowLink *EspNowLink::instance = nullptr;

EspNowLink::EspNowLink()
{
    instance = this;
}

EspNowLink::~EspNowLink()
{
    if (instance == this) instance = nullptr;
}

bool EspNowLink::init()
{
    // NVS may already be initialised by app_main; tolerate either.
    esp_err_t nvsRet = nvs_flash_init();
    if (nvsRet == ESP_ERR_NVS_NO_FREE_PAGES || nvsRet == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_wifi_set_protocol(
        WIFI_IF_STA,
        WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N));
    esp_wifi_config_80211_tx_rate(WIFI_IF_STA, WIFI_PHY_RATE_54M);

    if (esp_now_init() != ESP_OK) {
        ESP_LOGE(TAG, "esp_now_init failed");
        return false;
    }

    esp_now_register_recv_cb(&EspNowLink::onRecvTrampoline);
    esp_now_register_send_cb(&EspNowLink::onSentTrampoline);

    // Restore the bonded peer if present.
    if (loadPeerMacFromNvs() && !isAllZero(peerMac)) {
        esp_now_peer_info_t peerInfo = {};
        std::memcpy(peerInfo.peer_addr, peerMac, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;

        if (esp_now_add_peer(&peerInfo) == ESP_OK) {
            peerBonded = true;
            ESP_LOGI(TAG, "Restored bonded peer %02x:%02x:%02x:%02x:%02x:%02x",
                     peerMac[0], peerMac[1], peerMac[2],
                     peerMac[3], peerMac[4], peerMac[5]);
        } else {
            ESP_LOGW(TAG, "Failed to register stored peer, dropping it");
            std::memset(peerMac, 0, 6);
            peerBonded = false;
        }
    }

    ESP_LOGI(TAG, "ESP-NOW initialised (controller side)");
    return true;
}

bool EspNowLink::addBroadcastPeer()
{
    if (esp_now_is_peer_exist(BROADCAST_MAC)) return true;

    esp_now_peer_info_t peerInfo = {};
    std::memcpy(peerInfo.peer_addr, BROADCAST_MAC, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    esp_err_t ret = esp_now_add_peer(&peerInfo);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add broadcast peer: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool EspNowLink::bondPeer(const uint8_t mac[6])
{
    if (!esp_now_is_peer_exist(mac)) {
        esp_now_peer_info_t peerInfo = {};
        std::memcpy(peerInfo.peer_addr, mac, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;
        esp_err_t ret = esp_now_add_peer(&peerInfo);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "esp_now_add_peer failed: %s", esp_err_to_name(ret));
            return false;
        }
    }

    std::memcpy(peerMac, mac, 6);
    peerBonded = true;
    savePeerMacToNvs();
    return true;
}

void EspNowLink::forgetPeer()
{
    erasePeerMacFromNvs();

    // Wipe every known peer to be safe.
    esp_now_peer_info_t peerInfo = {};
    while (esp_now_fetch_peer(true, &peerInfo) == ESP_OK) {
        esp_now_del_peer(peerInfo.peer_addr);
    }

    std::memset(peerMac, 0, 6);
    peerBonded = false;
    ESP_LOGI(TAG, "Bonded peer forgotten");
}

esp_err_t EspNowLink::sendControllerRequest(const ControllerRequestDTO &dto)
{
    if (!peerBonded) {
        ESP_LOGW(TAG, "Not paired, dropping controller request");
        return ESP_FAIL;
    }
    ControllerRequestData wire = dto.toStruct();
    esp_err_t ret = esp_now_send(peerMac,
                                 reinterpret_cast<const uint8_t*>(&wire),
                                 sizeof(wire));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "sendControllerRequest failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t EspNowLink::sendPing()
{
    if (!peerBonded) return ESP_FAIL;
    PingRequestDTO ping{true};
    esp_err_t ret = esp_now_send(peerMac,
                                 reinterpret_cast<const uint8_t*>(&ping),
                                 sizeof(ping));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "sendPing failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t EspNowLink::sendPairingResponse()
{
    if (!peerBonded) return ESP_FAIL;
    PairingPacket pkt = {};
    std::strncpy(pkt.magic, RESP_MAGIC, sizeof(pkt.magic));
    return esp_now_send(peerMac,
                        reinterpret_cast<const uint8_t*>(&pkt),
                        sizeof(pkt));
}

esp_err_t EspNowLink::sendToBroadcast(const void *data, size_t len)
{
    return esp_now_send(BROADCAST_MAC,
                        static_cast<const uint8_t*>(data),
                        len);
}

bool EspNowLink::hasBondedPeer() const
{
    return peerBonded;
}

// ---------------------------------------------------------------------
// C-callback trampolines + dispatch
// ---------------------------------------------------------------------

void EspNowLink::onRecvTrampoline(const esp_now_recv_info_t *info,
                                  const uint8_t *data, int len)
{
    if (instance) instance->dispatchRecv(info, data, len);
}

void EspNowLink::onSentTrampoline(const uint8_t *macAddr,
                                  esp_now_send_status_t status)
{
    if (status != ESP_NOW_SEND_SUCCESS) {
        ESP_LOGW(TAG,
                 "TX failed to %02x:%02x:%02x:%02x:%02x:%02x (status %d)",
                 macAddr[0], macAddr[1], macAddr[2],
                 macAddr[3], macAddr[4], macAddr[5],
                 static_cast<int>(status));
    }
}

void EspNowLink::dispatchRecv(const esp_now_recv_info_t *info,
                              const uint8_t *data, int len)
{
    // Dispatch by payload size — the wire types are fixed-width structs.
    if (len == sizeof(PairingPacket)) {
        if (onPairingPacket) {
            PairingPacket pkt;
            std::memcpy(&pkt, data, sizeof(pkt));
            onPairingPacket(pkt, info->src_addr);
        }
    } else if (len == sizeof(PingRequestDTO)) {
        if (onPing) {
            PingRequestDTO pkt;
            std::memcpy(&pkt, data, sizeof(pkt));
            onPing(pkt);
        }
    } else if (len == sizeof(mpuDTO)) {
        if (onMpu) {
            mpuDTO pkt;
            std::memcpy(&pkt, data, sizeof(pkt));
            onMpu(pkt);
        }
    } else if (len == sizeof(ControllerRequestData)) {
        if (onControllerData) {
            ControllerRequestData pkt;
            std::memcpy(&pkt, data, sizeof(pkt));
            onControllerData(pkt);
        }
    } else {
        ESP_LOGD(TAG, "Unknown packet of length %d ignored", len);
    }
}

// ---------------------------------------------------------------------
// NVS persistence
// ---------------------------------------------------------------------

bool EspNowLink::loadPeerMacFromNvs()
{
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) return false;
    size_t size = sizeof(peerMac);
    esp_err_t err = nvs_get_blob(handle, NVS_KEY_PEER, peerMac, &size);
    nvs_close(handle);
    return err == ESP_OK;
}

bool EspNowLink::savePeerMacToNvs()
{
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) return false;
    esp_err_t err = nvs_set_blob(handle, NVS_KEY_PEER, peerMac, sizeof(peerMac));
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    return err == ESP_OK;
}

bool EspNowLink::erasePeerMacFromNvs()
{
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) return false;
    nvs_erase_key(handle, NVS_KEY_PEER);
    nvs_commit(handle);
    nvs_close(handle);
    return true;
}

} // namespace Aerisys::Controller
