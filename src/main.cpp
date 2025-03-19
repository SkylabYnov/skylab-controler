#include "./feature/joysticksManager/JoysticksManager.h"
#include "./feature/buttonsManager/ButtonsManager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include <esp_mac.h>


JoysticksManager* joysticksManager;
ButtonsManager* buttonsManager;

extern "C" void app_main() {
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    
    ESP_LOGI("MAC", "Adresse MAC : %02X:%02X:%02X:%02X:%02X:%02X", 
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());


    EspNowHandler* espNow = new EspNowHandler();
    if (!espNow->init()) {
        ESP_LOGE("MAIN", "ESP-NOW init failed!");
        return;
    }
    
    joysticksManager = new JoysticksManager(espNow);
    joysticksManager->initJoystick();
    buttonsManager = new ButtonsManager(espNow);
    buttonsManager->initButton();

    xTaskCreate([](void*) { joysticksManager->Task(); },
                "joystickManagerTask", 4096, &joysticksManager, 5, nullptr);

    xTaskCreate([](void*) { buttonsManager->Task(); },
                "buttonManagerTask", 2048, &buttonsManager, 5, nullptr);
}
