#include "WifiServer.h"
#include <string.h>

const char* WifiServer::Tag = "WifiServer";

WifiServer::WifiServer(const std::string& ssid, const std::string& password)
    : ssid(ssid), password(password) {}

void WifiServer::Init() {
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifiConfig = {};
    strncpy(reinterpret_cast<char*>(wifiConfig.ap.ssid), ssid.c_str(), ssid.length());
    strncpy(reinterpret_cast<char*>(wifiConfig.ap.password), password.c_str(), password.length());
    wifiConfig.ap.ssid_len = ssid.length();
    wifiConfig.ap.channel = 1;
    wifiConfig.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    wifiConfig.ap.max_connection = 4;
    wifiConfig.ap.beacon_interval = 100;

    if (password.empty()) {
        wifiConfig.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifiConfig));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(Tag, "Point d'accès initialisé avec SSID: %s", ssid.c_str());
}
