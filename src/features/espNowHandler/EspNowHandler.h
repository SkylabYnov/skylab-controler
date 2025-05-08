#ifndef ESP_NOW_HANDLER_H
#define ESP_NOW_HANDLER_H

#include <ControllerRequestDTO.h>

#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <string.h>
#include <nvs_flash.h>

#define ESP_MAC {0xAC, 0x15, 0x18, 0xE6, 0x35, 0x68}  // MAC du Drone
// #define ESP_MAC {0xA0, 0xDD, 0x6C, 0x10, 0x3E, 0x34}  // MAC ESP Max

class EspNowHandler {
public:
    EspNowHandler();
    ~EspNowHandler();

    bool init();
    void send_data(const ControllerRequestDTO& requestDto);

private:

    static uint8_t peer_mac[6];  
};

#endif // ESP_NOW_HANDLER_H
