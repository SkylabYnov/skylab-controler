#ifndef WIFI_SERVER_H
#define WIFI_SERVER_H

#include <string>
#include "esp_wifi.h"
#include "esp_log.h"

class WifiServer {
public:
    WifiServer(const std::string& ssid, const std::string& password);
    void Init();
    std::string getDroneIp();

private:
    
    std::string ssid;
    std::string password;
    static const char *Tag;
    static std::string droneIp;
    static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    
};

#endif // WIFI_SERVER_H
