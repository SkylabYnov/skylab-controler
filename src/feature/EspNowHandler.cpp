#include "EspNowHandler.h"

#include <cstring>
// Ces DTOs/Packets sont nécessaires. Assurez-vous qu'ils sont définis et inclus.
#include <PairingPacket.h> 
#include <PingRequestDTO.h>
#include <ControllerRequestDTO.h> 
#include <esp_system.h> // Ajout pour esp_restart()

#include <esp_mac.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <esp_now.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAG "ESP_NOW_CONTROLLER"
#define PAIRING_LED_GPIO GPIO_NUM_2
#define BUTTON_GPIO GPIO_NUM_16

EspNowHandler *EspNowHandler::instance = nullptr;

EspNowHandler::EspNowHandler() = default;
EspNowHandler::~EspNowHandler() = default;

// ... La fonction init() est conservée telle quelle, car la logique principale est correcte.
// Seul le commentaire sur _associationMode est ajusté.

bool EspNowHandler::init() {
    instance = this;

    // 1. Initialisation WiFi (Mode Station)
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // 2. Initialisation ESP-NOW
    if (esp_now_init() != ESP_OK) {
        ESP_LOGE(TAG, "Erreur d'init ESP-NOW");
        return false;
    }

    // 3. Configuration GPIO (LED et bouton)
    gpio_reset_pin(PAIRING_LED_GPIO);
    gpio_set_direction(PAIRING_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(PAIRING_LED_GPIO, 0);

    gpio_reset_pin(BUTTON_GPIO);
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_GPIO, GPIO_PULLUP_ONLY);

    // 4. Chargement MAC depuis NVS
    bool macLoaded = loadPeerMacFromNvs();
    
    // Vérification si le MAC est "non-nul"
    bool macKnown = false;
    for (int i = 0; i < 6; i++) {
        if (peer_mac[i] != 0) {
            macKnown = true;
            break;
        }
    }

    // 5. Tentative d'ajout du pair et définition du statut
    if (macLoaded && macKnown) {
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, peer_mac, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;

        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            ESP_LOGE(TAG, "Erreur d'ajout du pair connu. Redémarrage du mode appairage.");
            _associationMode = false; // Non associé = mode appairage
            start_pairing();
        } else {
            _associationMode = true; // Associé
            ESP_LOGI(TAG, "Pair connu ajouté : %02x:%02x:%02x:%02x:%02x:%02x", 
                     peer_mac[0], peer_mac[1], peer_mac[2], peer_mac[3], peer_mac[4], peer_mac[5]);
        }
    } else {
        _associationMode = false; // Non associé = mode appairage
        ESP_LOGW(TAG, "Aucun pair connu. Démarrage du mode appairage.");
        start_pairing();
    }

    // 6. Tâche qui gère le bouton (toujours active)
    xTaskCreate(button_task, "button_task", 2048, nullptr, 1, nullptr);

    // 7. Callback Réception ESP-NOW
    esp_now_register_recv_cb([](const esp_now_recv_info_t *info, const uint8_t *data, int len) {
        if (!instance) return;

        // Si on n'est PAS associé (mode appairage)
        if (!instance->_associationMode) { 
            if (len == sizeof(PairingPacket)) {
                PairingPacket pkt;
                memcpy(&pkt, data, sizeof(pkt));

                if (memcmp(pkt.magic, "AERISYS_DRONE_PAIR", 18) == 0) {
                    instance->_associationMode = false;
                    ESP_LOGI(TAG, "Paquet d'appairage reçu !");
                    
                    memcpy(instance->peer_mac, info->src_addr, 6);
                    instance->savePeerMacToNvs();

                    esp_now_peer_info_t peer = {};
                    memcpy(peer.peer_addr, info->src_addr, 6);
                    peer.channel = 0;
                    peer.encrypt = false;
                    if (!esp_now_is_peer_exist(info->src_addr)) esp_now_add_peer(&peer);

                     // Confirmation d'appairage réussi
                    ESP_LOGI(TAG, "Appairage réussi !");

                    // Arrêt des tâches d'appairage (LED et Broadcast)
                    if (instance->_pairingLedTaskHandle) vTaskDelete(instance->_pairingLedTaskHandle);
                    if (instance->_pairingBroadcastTaskHandle) vTaskDelete(instance->_pairingBroadcastTaskHandle);
                    instance->_pairingLedTaskHandle = nullptr;
                    instance->_pairingBroadcastTaskHandle = nullptr;

                    // Envoi de la confirmation au drone
                    PairingPacket confirm_dto = {};
                    strncpy(confirm_dto.magic, "PAIR_CONFIRM", sizeof(confirm_dto.magic));
                    esp_read_mac(confirm_dto.mac, ESP_MAC_WIFI_STA); 

                    esp_err_t result = esp_now_send(instance->peer_mac, (uint8_t *) &confirm_dto, sizeof(confirm_dto));

                    if (result == ESP_OK) {
                        ESP_LOGI(TAG, "Confirmation d'association envoyée");
                    } else {
                        ESP_LOGE(TAG, "Erreur envoi confirmation : %s", esp_err_to_name(result));
                    }
                }
            }
        } 
        // Si on est associé
        else {
             if (len == sizeof(PingRequestDTO)) {
                PingRequestDTO ping;
                memcpy(&ping, data, sizeof(ping));
                ESP_LOGI(TAG, "Ping/Statut reçu : %s", ping.pingState ? "ON" : "OFF");
            }
        }
    });

    // 8. Callback envoi (pour le debug)
    esp_now_register_send_cb([](const uint8_t *mac, esp_now_send_status_t status) {
        if (status != ESP_NOW_SEND_SUCCESS) {
            ESP_LOGW(TAG, "Envoi au MAC %02x:%02x:%02x:%02x:%02x:%02x: Échec", 
                      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        }
    });

    ESP_LOGI(TAG, "ESP-NOW Initialisé (manette)");
    return true;
}

// --- Tâches FreeRTOS ---

void EspNowHandler::button_task(void *pv) {
    bool last_pressed = false;
    while (true) {
        int current_level = gpio_get_level(BUTTON_GPIO);
        bool currently_pressed = (current_level == 0);

        if (currently_pressed && !last_pressed && instance) {
            if (instance->_associationMode) {
                // Si associé -> Reset et redémarrage du pairing
                ESP_LOGW(TAG, "Bouton: Reset association demandé.");
                instance->resetAssociation();
            } else {
                // Si non associé -> Démarrage du pairing (si pas déjà en cours)
                ESP_LOGW(TAG, "Bouton: Démarrage appairage (si non en cours).");
                instance->start_pairing();
            }
        }

        last_pressed = currently_pressed;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void EspNowHandler::start_pairing() {
    // Si déjà associé, on ne fait rien
    if (_associationMode) { 
        ESP_LOGI(TAG, "Déjà associé, ignorer start_pairing.");
        return;
    }
    
    // Si les tâches existent déjà (déjà en pairing), on ne fait rien
    if (_pairingLedTaskHandle || _pairingBroadcastTaskHandle) {
        ESP_LOGI(TAG, "Déjà en mode appairage. Ne relance pas les tâches.");
        return;
    }

    ESP_LOGI(TAG, "Démarrage appairage...");

    // LED clignotante
    xTaskCreate(pairing_led_task, "pairing_led_task", 2048, nullptr, 1, &_pairingLedTaskHandle);
    // Broadcast pairing
    xTaskCreate(pairing_broadcast_task, "pairing_broadcast_task", 4096, nullptr, 1, &_pairingBroadcastTaskHandle);
}

// 💥 CORRECTION MAJEURE ICI 💥
void EspNowHandler::pairing_led_task(void *pv) {
    // La boucle continue tant que l'instance existe ET qu'on n'est PAS associé
    while (instance && !instance->_associationMode) { 
        gpio_set_level(PAIRING_LED_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(300));
        gpio_set_level(PAIRING_LED_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
    gpio_set_level(PAIRING_LED_GPIO, 0); // Éteindre la LED à la fin de l'appairage
    instance->_pairingLedTaskHandle = nullptr; 
    vTaskDelete(nullptr);
}

// 🛠️ CORRECTION DE LA BOUCLE ICI 🛠️
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
    if (!esp_now_is_peer_exist(broadcastAddress)) {
        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            ESP_LOGE(TAG, "Erreur ajout pair Broadcast");
            instance->_pairingBroadcastTaskHandle = nullptr;
            vTaskDelete(nullptr);
            return;
        }
    }

    // La boucle continue tant que l'instance existe ET qu'on n'est PAS associé
    while (instance && !instance->_associationMode) { 
        PairingPacket dto = {};
        strncpy(dto.magic, "PAIR_CONFIRM", sizeof(dto.magic)); 
        esp_read_mac(dto.mac, ESP_MAC_WIFI_STA);
        esp_now_send(broadcastAddress, (uint8_t *) &dto, sizeof(dto));
        ESP_LOGD(TAG, "Broadcast appairage envoyé.");
        vTaskDelay(pdMS_TO_TICKS(1000)); // Envoie le paquet toutes les 1 seconde
    }

    // Suppression du pair broadcast après l'appairage (Optionnel, mais bonne pratique)
    esp_now_del_peer(broadcastAddress);

    instance->_pairingBroadcastTaskHandle = nullptr; 
    vTaskDelete(nullptr);
}

// --- Gestion NVS ---
// Fonctions loadPeerMacFromNvs, savePeerMacToNvs, erasePeerMacFromNvs (implémentation conservée)

bool EspNowHandler::loadPeerMacFromNvs() {
    nvs_handle_t handle;
    if (nvs_open("storage", NVS_READONLY, &handle) != ESP_OK) return false;
    size_t size = sizeof(peer_mac);
    esp_err_t err = nvs_get_blob(handle, "controller_mac", peer_mac, &size);
    nvs_close(handle);
    return err == ESP_OK;
}

bool EspNowHandler::savePeerMacToNvs() {
    nvs_handle_t handle;
    if (nvs_open("storage", NVS_READWRITE, &handle) != ESP_OK) return false;
    ESP_ERROR_CHECK(nvs_set_blob(handle, "controller_mac", peer_mac, sizeof(peer_mac)));
    ESP_ERROR_CHECK(nvs_commit(handle));
    nvs_close(handle);
    return true;
}

void EspNowHandler::erasePeerMacFromNvs() {
     nvs_handle_t handle;
    if (nvs_open("storage", NVS_READWRITE, &handle) == ESP_OK) {
        nvs_erase_key(handle, "controller_mac");
        nvs_commit(handle);
        nvs_close(handle);
        ESP_LOGI(TAG, "Ancienne MAC du pair effacée de la NVS.");
    }
}


void EspNowHandler::resetAssociation() {
    ESP_LOGW(TAG, "Réinitialisation de l'association...");

    // 2. Suppression de la MAC de la NVS
    erasePeerMacFromNvs();

    // 3. Suppression du pair connu
    if (esp_now_is_peer_exist(peer_mac)) {
        esp_now_del_peer(peer_mac); 
    }

    // 4. Réinitialiser la MAC locale
    memset(peer_mac, 0, 6); 

    // 5. Désactivation du mode associé et démarrage du pairing
    _associationMode = false;
    start_pairing();
}

// --- Envoi de données ---

void EspNowHandler::send_data(const ControllerRequestDTO &controllerRequestDTO) {
    if (!_associationMode) { 
        ESP_LOGW(TAG, "Drone non appairé ! Impossible d'envoyer les données de contrôle.");
        return;
    }

    ControllerRequestData requestData = controllerRequestDTO.toStruct();
    esp_err_t result = esp_now_send(peer_mac, (uint8_t *) &requestData, sizeof(requestData));
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Erreur d'envoi des données : %s", esp_err_to_name(result));
    }
}

void EspNowHandler::send_ping() {
    if (!_associationMode) { 
        ESP_LOGW(TAG, "Drone non appairé ! Impossible d'envoyer le ping.");
        return;
    }

    PingRequestDTO ping = {true}; 
    esp_err_t result = esp_now_send(peer_mac, (uint8_t *) &ping, sizeof(ping));
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Erreur d'envoi du ping : %s", esp_err_to_name(result));
    }
}