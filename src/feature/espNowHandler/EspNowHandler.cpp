#include "EspNowHandler.h"

#define TAG "ESP_NOW"

uint8_t EspNowHandler::peer_mac[6] = ESP_MAC;

EspNowHandler::EspNowHandler() {}

EspNowHandler::~EspNowHandler() {}

bool EspNowHandler::init() {
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    if (esp_now_init() != ESP_OK) {
        ESP_LOGE(TAG, "Erreur d'init ESP-NOW");
        return false;
    }

    esp_now_register_recv_cb([](const esp_now_recv_info_t *info, const uint8_t *data, int len) {
        ESP_LOGI(TAG, "Données reçues !");
    });

    esp_now_register_send_cb([](const uint8_t *macAddr, esp_now_send_status_t status) {
        ESP_LOGI(TAG, "Envoi: %s", status == ESP_NOW_SEND_SUCCESS ? "Succès" : "Échec");
    });

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, peer_mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        ESP_LOGE(TAG, "Erreur d'ajout du pair");
        return false;
    }

    ESP_LOGI(TAG, "ESP-NOW Initialisé");
    return true;
}

void EspNowHandler::send_data(const ControllerRequestDTO& controllerRequestDTO) {
    ControllerRequestData requestData = controllerRequestDTO.toStruct();
    if (esp_now_send(peer_mac, (uint8_t*)&requestData, sizeof(requestData)) != ESP_OK) {
        ESP_LOGI(TAG, "Erreur d'envoi : %s", controllerRequestDTO.toString().c_str());
    } else {
        
        ESP_LOGI(TAG, "Données envoyées : %s", controllerRequestDTO.toString().c_str());

    }
}
