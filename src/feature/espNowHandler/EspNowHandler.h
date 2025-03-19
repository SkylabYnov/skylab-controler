#ifndef ESP_NOW_HANDLER_H
#define ESP_NOW_HANDLER_H

#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <string.h>
#include <ControllerRequestDTO.h>
#include <nvs_flash.h>

#define ESP_MAC {0x24, 0x6F, 0x28, 0xA1, 0xB2, 0xC3}  // MAC du Drone

class EspNowHandler {
public:
    EspNowHandler();
    ~EspNowHandler();

    bool init();
    void send_data(const ControllerRequestData& requestData);

private:

    static uint8_t peer_mac[6];  
};

#endif // ESP_NOW_HANDLER_H
