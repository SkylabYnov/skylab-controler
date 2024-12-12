#ifndef WIFI_SERVER_H
#define WIFI_SERVER_H

#include <string>
#include "esp_wifi.h"
#include "esp_log.h"

class WifiServer {
public:
    WifiServer(const std::string& ssid, const std::string& password);
    void Init();

private:
    std::string ssid;
    std::string password;
    static const char *Tag;
};

#endif // WIFI_SERVER_H
