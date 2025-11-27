#ifndef ESP_NOW_HANDLER_H
#define ESP_NOW_HANDLER_H

#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ControllerRequestDTO.h>

#define PIN_LED_ASSOCIATION GPIO_NUM_2

#define PIN_BUTTON_ASSOCIATION GPIO_NUM_16

class EspNowHandler {
public:
    EspNowHandler();
    ~EspNowHandler();

    bool init();
    void start_pairing();
    void send_data(const ControllerRequestDTO &requestDto);
    void send_ping();
    static void Task(void* pvParameter);

private:
    static void IRAM_ATTR button_isr_handler_pairing(void *arg);
    volatile bool buttonPressedPairing = false;
    // Méthodes NVS
    bool loadPeerMacFromNvs();
    bool savePeerMacToNvs();
    void updateAssociationLed();
    void resetAssociation();

    // Singleton instance
    static EspNowHandler* instance;

    uint8_t peer_mac[6]{};
    bool _associationMode = false; // Remplacé _associationMode par _associationMode pour une meilleure sémantique
    bool currentLedState = false;
    static int64_t lastToggleTimeUs;
};

#endif // ESP_NOW_HANDLER_H