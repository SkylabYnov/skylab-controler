#include "./feature/joysticksManager/JoysticksManager.h"
#include "./feature/buttonsManager/ButtonsManager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"


JoysticksManager* joysticksManager;
ButtonsManager* buttonsManager;

extern "C" void app_main() {
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
                "buttonManagerTask", 4096, &buttonsManager, 5, nullptr);
}
