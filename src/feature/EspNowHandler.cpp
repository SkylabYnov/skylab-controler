#include "EspNowHandler.h"

#include <cstring>
#include <PairingPacket.h>
#include <PingRequestDTO.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <esp_now.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAG "ESP_NOW_CONTROLLER"
#define PAIRING_LED_GPIO GPIO_NUM_2
#define BUTTON_GPIO GPIO_NUM_0

EspNowHandler *EspNowHandler::instance = nullptr;
bool EspNowHandler::button_pressed_flag = false;

EspNowHandler::EspNowHandler() = default;
EspNowHandler::~EspNowHandler() = default;

bool EspNowHandler::init() {
    instance = this;

    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    if (esp_now_init() != ESP_OK) {
        ESP_LOGE(TAG, "Erreur d'init ESP-NOW");
        return false;
    }

    // LED et bouton
    gpio_reset_pin(PAIRING_LED_GPIO);
    gpio_set_direction(PAIRING_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(PAIRING_LED_GPIO, 0);

    gpio_reset_pin(BUTTON_GPIO);
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_GPIO, GPIO_PULLUP_ONLY);

    // ISR bouton
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, nullptr);

    // Tâche qui gère le bouton
    xTaskCreate(button_task, "button_task", 2048, nullptr, 1, nullptr);

    // Réception ESP-NOW
    esp_now_register_recv_cb([](const esp_now_recv_info_t *info, const uint8_t *data, int len) {
        if (!instance) return;

        if (instance->isPaired) {
            if (len == sizeof(PingRequestDTO)) {
                PingRequestDTO ping;
                memcpy(&ping, data, sizeof(ping));
                ESP_LOGI(TAG, "Ping reçu : %s", ping.pingState ? "ON" : "OFF");
            }
            return;
        }

        if (len == sizeof(PairingPacket)) {
            PairingPacket pkt;
            memcpy(&pkt, data, sizeof(pkt));

            if (memcmp(pkt.magic, "AERISYS_DRONE_PAIR", 18) == 0) {
                ESP_LOGI(TAG, "Paquet d'appairage reçu !");
                memcpy(instance->peer_mac, info->src_addr, 6);

                esp_now_peer_info_t peer = {};
                memcpy(peer.peer_addr, info->src_addr, 6);
                peer.channel = 0;
                peer.encrypt = false;
                if (!esp_now_is_peer_exist(info->src_addr)) esp_now_add_peer(&peer);

                instance->isPaired = true;
                instance->isPairing = false;
                ESP_LOGI(TAG, "Appairage réussi !");

                PairingPacket dto;
                memset(&dto, 0, sizeof(dto)); // Bonnes pratiques : on met tout à zéro d'abord
                strncpy(dto.magic, "PAIR_CONFIRM", sizeof(dto.magic));
                esp_read_mac(dto.mac, ESP_MAC_WIFI_STA);

                esp_err_t result = esp_now_send(instance->peer_mac, (uint8_t *) &dto, sizeof(dto));

                if (result == ESP_OK) {
                    ESP_LOGI(TAG, "Demande d'association envoyée (Broadcast)");
                } else {
                    ESP_LOGE(TAG, "Erreur envoi association : %s", esp_err_to_name(result));
                }
            }
        }
    });

    // Callback envoi
    esp_now_register_send_cb([](const uint8_t *mac, esp_now_send_status_t status) {
        if (status != ESP_NOW_SEND_SUCCESS) {
            ESP_LOGI(TAG, "Envoi: Échec");
        }
    });

    ESP_LOGI(TAG, "ESP-NOW Initialisé (manette) - en attente pairing");
    return true;
}

// Tâche ISR bouton
void IRAM_ATTR EspNowHandler::button_isr_handler(void *arg) {
    button_pressed_flag = true;
}

void EspNowHandler::button_task(void *pv) {
    while (true) {
        if (button_pressed_flag && instance) {
            button_pressed_flag = false;
            instance->on_button_pressed();
        }
        vTaskDelay(pdMS_TO_TICKS(50)); // anti-rebond simple
    }
}

void EspNowHandler::on_button_pressed() {
    if (!isPaired) {
        start_pairing();
    }
}

void EspNowHandler::start_pairing() {
    if (isPaired || isPairing) return;

    isPairing = true;
    ESP_LOGI(TAG, "Démarrage appairage...");

    // LED clignotante
    xTaskCreate(pairing_led_task, "pairing_led_task", 2048, nullptr, 1, nullptr);
    // Broadcast pairing
    xTaskCreate(pairing_broadcast_task, "pairing_broadcast_task", 4096, nullptr, 1, nullptr);
}

void EspNowHandler::pairing_led_task(void *pv) {
    while (instance && instance->isPairing) {
        gpio_set_level(PAIRING_LED_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(300));
        gpio_set_level(PAIRING_LED_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
    gpio_set_level(PAIRING_LED_GPIO, 0);
    vTaskDelete(nullptr);
}

void EspNowHandler::pairing_broadcast_task(void *pv) {
    if (!instance) {
        vTaskDelete(nullptr);
        return;
    }

    uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    if (!esp_now_is_peer_exist(broadcastAddress)) esp_now_add_peer(&peerInfo);

    while (instance->isPairing && !instance->isPaired) {
        PairingPacket dto = {};
        strncpy(dto.magic, "PAIR_CONFIRM", sizeof(dto.magic));
        esp_read_mac(dto.mac, ESP_MAC_WIFI_STA);
        esp_now_send(broadcastAddress, (uint8_t *) &dto, sizeof(dto));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(nullptr);
}

// Envoi données
void EspNowHandler::send_data(const ControllerRequestDTO &controllerRequestDTO) {
    if (!isPaired) {
        ESP_LOGW(TAG, "Drone non appairé !");
        return;
    }

    ControllerRequestData requestData = controllerRequestDTO.toStruct();
    esp_now_send(peer_mac, (uint8_t *) &requestData, sizeof(requestData));
}

void EspNowHandler::send_ping() {
    if (!isPaired) {
        ESP_LOGW(TAG, "Drone non appairé !");
        return;
    }

    PingRequestDTO ping = {true};
    esp_now_send(peer_mac, (uint8_t *) &ping, sizeof(ping));
}
