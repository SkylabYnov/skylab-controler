#include "WifiServer.h"
#include <string.h>
#include <lwip/ip4_addr.h>

const char* WifiServer::Tag = "WifiServer";
std::string WifiServer::droneIp = "";

WifiServer::WifiServer(const std::string& ssid, const std::string& password)
    : ssid(ssid), password(password) {}

void WifiServer::Init() {
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &WifiServer::wifi_event_handler, this));

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

std::string WifiServer::getDroneIp()
{
    return WifiServer::droneIp;
}

void WifiServer::wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_id == IP_EVENT_AP_STAIPASSIGNED) {
        ip_event_ap_staipassigned_t *event = (ip_event_ap_staipassigned_t *)event_data;

        ESP_LOGI(Tag, "Adresse IP assignée : " IPSTR, IP2STR(&event->ip));

        char ipStr[IP4ADDR_STRLEN_MAX];
        snprintf(ipStr, IP4ADDR_STRLEN_MAX, IPSTR, IP2STR(&event->ip));
        WifiServer::droneIp = std::string(ipStr);
        ESP_LOGI(Tag, "Adresse IP assignée : %s", std::string(ipStr).c_str());
    }
}
