#ifndef ESP_NOW_HANDLER_H
#define ESP_NOW_HANDLER_H

#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ControllerRequestDTO.h>
#include <esp_now.h>

#define PIN_LED_ASSOCIATION GPIO_NUM_2

#define PIN_BUTTON_ASSOCIATION GPIO_NUM_16

#define LONG_PRESS_MS 5000

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
    volatile bool buttonPressed = false;
    volatile bool buttonLogPressedSucess = false;
    volatile int64_t pressStartTime = 0;
    void handleButtonPressLogic();

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

    static void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len);
    static void onDataSent(const uint8_t *macAddr, esp_now_send_status_t status);
};

#endif // ESP_NOW_HANDLER_H