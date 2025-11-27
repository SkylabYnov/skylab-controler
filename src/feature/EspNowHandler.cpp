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
#include <esp_timer.h>

#define TAG "ESP_NOW_CONTROLLER"

EspNowHandler *EspNowHandler::instance = nullptr;
int64_t EspNowHandler::lastToggleTimeUs = 0;

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
    gpio_reset_pin(PIN_LED_ASSOCIATION);
    gpio_set_direction(PIN_LED_ASSOCIATION, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_LED_ASSOCIATION, 0);

    gpio_reset_pin(PIN_BUTTON_ASSOCIATION);
    gpio_set_direction(PIN_BUTTON_ASSOCIATION, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_BUTTON_ASSOCIATION, GPIO_PULLUP_ONLY);

    // 🔥 Ajout important
    gpio_set_intr_type(PIN_BUTTON_ASSOCIATION, GPIO_INTR_NEGEDGE);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_BUTTON_ASSOCIATION, button_isr_handler_pairing, this);


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
            _associationMode = true; // Non associé = mode appairage
            start_pairing();
        } else {
            _associationMode = false; // Associé
            ESP_LOGI(TAG, "Pair connu ajouté : %02x:%02x:%02x:%02x:%02x:%02x", 
                     peer_mac[0], peer_mac[1], peer_mac[2], peer_mac[3], peer_mac[4], peer_mac[5]);
        }
    } else {
        _associationMode = true; // Non associé = mode appairage
        ESP_LOGW(TAG, "Aucun pair connu. Démarrage du mode appairage.");
        start_pairing();
    }

    // 7. Callback Réception ESP-NOW
    esp_now_register_recv_cb([](const esp_now_recv_info_t *info, const uint8_t *data, int len) {
        if (!instance) return;

        // Si on n'est PAS associé (mode appairage)
        if (instance->_associationMode) { 
            if (len == sizeof(PairingPacket)) {
                PairingPacket pkt;
                memcpy(&pkt, data, sizeof(pkt));

                if (strncmp(pkt.magic, "AERISYS_DRONE_PAIR", sizeof(pkt.magic)) == 0){
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


void EspNowHandler::start_pairing() {
    // Si on est déjà en appairage, ne PAS bloquer → on continue
    if (_associationMode) {
        ESP_LOGI(TAG, "Déjà en mode appairage.");
    }

    _associationMode = true;

    uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (!esp_now_is_peer_exist(broadcastAddress)) {
        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            ESP_LOGE(TAG, "Erreur ajout pair Broadcast");
            return;
        }
    }

    ESP_LOGI(TAG, "Mode appairage ACTIVÉ (écoute broadcast)");
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

void EspNowHandler::resetAssociation() {
    // 1. Suppression de la MAC de la NVS
    // Note: Pour une suppression complète, il faut effacer la clé dans NVS
    nvs_handle_t handle;
    if (nvs_open("storage", NVS_READWRITE, &handle) == ESP_OK) {
        nvs_erase_key(handle, "controller_mac");
        nvs_commit(handle);
        nvs_close(handle);
        ESP_LOGI(TAG, "Ancienne MAC du pair effacée de la NVS.");
    }

    // 2. Suppression de tous les pairs ESP-NOW
    esp_now_del_peer(peer_mac); // Supprime l'ancien pair connu (si existant)

    // Optionnel : Effacer le reste de la liste des pairs (y compris l'adresse de broadcast si elle n'est plus nécessaire)
    esp_now_peer_info_t peerInfo = {};
    while (esp_now_fetch_peer(true, &peerInfo) == ESP_OK) {
        esp_now_del_peer(peerInfo.peer_addr);
    }
    ESP_LOGI(TAG, "Tous les pairs ESP-NOW existants ont été supprimés.");

    // 3. Réinitialiser la MAC locale (pour l'affichage futur)
    memset(peer_mac, 0, 6); 

    // 4. Activation du mode association
    _associationMode = true;
}

void EspNowHandler::updateAssociationLed() {
    int64_t now = esp_timer_get_time();
    
    // --- Étape 1 : Vérification du Mode Association ---
    if (_associationMode) {
        int64_t timeElapsed = now - EspNowHandler::lastToggleTimeUs;
        if (timeElapsed > 200000LL) {             
            gpio_set_level(PIN_LED_ASSOCIATION, !currentLedState);
            
            currentLedState = !currentLedState;
            EspNowHandler::lastToggleTimeUs = now;
            
            ESP_LOGD(TAG, "lastToggleTimeUs mis à jour à %lld.", EspNowHandler::lastToggleTimeUs);

        } else {
            ESP_LOGD(TAG, "LED Association : Attente pour le prochain basculement.");
        }
        
    } else {
        currentLedState = false;
        gpio_set_level(PIN_LED_ASSOCIATION, currentLedState); 
        ESP_LOGD(TAG, "LED Association : Mode Inactif (Éteinte)."); 
    }
}

void EspNowHandler::Task(void* pvParameter) 
{
    int64_t lastPingTime = esp_timer_get_time();
    EspNowHandler* instance = static_cast<EspNowHandler*>(pvParameter);
    while (true) {
        instance->updateAssociationLed();

        int64_t now = esp_timer_get_time();
        if (now - lastPingTime >= 1000000)
        {
            instance->send_ping();
            lastPingTime = now;
        }

        if (instance->buttonPressedPairing && instance->_associationMode == false) {
            ESP_LOGI(TAG, "Bouton d'association pressé, lancement du RESET d'association.");
            instance->resetAssociation();
            instance->buttonPressedPairing = false;
        }

        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}

void IRAM_ATTR EspNowHandler::button_isr_handler_pairing(void *arg)
{
    EspNowHandler *self = static_cast<EspNowHandler *>(arg);
    self->buttonPressedPairing = true;
}


// --- Envoi de données ---

void EspNowHandler::send_data(const ControllerRequestDTO &controllerRequestDTO) {
    if (_associationMode) { 
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
    if (_associationMode) { 
        ESP_LOGW(TAG, "Drone non appairé ! Impossible d'envoyer le ping.");
        return;
    }

    PingRequestDTO ping = {true}; 
    esp_err_t result = esp_now_send(peer_mac, (uint8_t *) &ping, sizeof(ping));
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Erreur d'envoi du ping : %s", esp_err_to_name(result));
    }
}