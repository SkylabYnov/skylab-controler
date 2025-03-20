#ifndef ESP_NOW_HANDLER_H
#define ESP_NOW_HANDLER_H

#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <string.h>
#include <ControllerRequestDTO.h>
#include <nvs_flash.h>

#define ESP_MAC {0xA0, 0xDD, 0x6C, 0x10, 0x3E, 0x34}  // MAC du Drone

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
