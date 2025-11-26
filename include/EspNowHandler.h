#ifndef ESP_NOW_HANDLER_H
#define ESP_NOW_HANDLER_H

#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ControllerRequestDTO.h>

class EspNowHandler {
public:
    EspNowHandler();
    ~EspNowHandler();

    /**
     * @brief Initialise le WiFi, ESP-NOW, les GPIOs et charge le MAC pair depuis NVS.
     * @return true si l'initialisation réussit, false sinon.
     */
    bool init();

    /**
     * @brief Démarre le mode appairage (LED clignotante et broadcast).
     */
    void start_pairing();

    /**
     * @brief Envoie les données de contrôle au pair (Drone).
     */
    void send_data(const ControllerRequestDTO &requestDto);
    
    /**
     * @brief Envoie un ping au pair.
     */
    void send_ping();

    /**
     * @brief Réinitialise l'appairage (supprime le MAC de NVS et les pairs ESP-NOW).
     */
    void resetAssociation();

private:
    // Tâches statiques (FreeRTOS)
    static void button_task(void* pv);
    static void pairing_led_task(void *pv);
    static void pairing_broadcast_task(void *pv);

    // Méthodes NVS
    bool loadPeerMacFromNvs();
    bool savePeerMacToNvs();
    void erasePeerMacFromNvs();

    // Singleton instance
    static EspNowHandler* instance;

    uint8_t peer_mac[6]{};
    bool _associationMode = false; // Remplacé _associationMode par _associationMode pour une meilleure sémantique
    
    TaskHandle_t _pairingLedTaskHandle = nullptr; // Handle pour la tâche LED
    TaskHandle_t _pairingBroadcastTaskHandle = nullptr; // Handle pour la tâche Broadcast
};

#endif // ESP_NOW_HANDLER_H