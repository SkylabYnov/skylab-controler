#include "JoysticksManager.h"
#include "ButtonsManager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "ReadComputer.h"

JoysticksManager *joysticksManager;
ButtonsManager *buttonsManager;

bool modeComputer = true;

extern "C" void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    EspNowHandler *espNow = new EspNowHandler();
    if (!espNow->init())
    {
        ESP_LOGE("MAIN", "ESP-NOW init failed!");
        return;
    }

    xTaskCreate(
        espNow->Task,           // Fonction d'entrée statique
        "espNowTask",         // Nom de la tâche
        4096,                   // Taille de la pile (en octets, souvent 4096 pour une tâche C++)
        espNow,                 // Argument : Pointeur 'this' vers l'instance
        1, // Priorité (élevée)
        NULL                    // Handle de tâche (non utilisé ici)
    );

    if(modeComputer){

        ReadComputer *reader = new ReadComputer(espNow, 115200);

        xTaskCreate(
        reader->Task,           // Fonction d'entrée statique
        "read_pc_task",         // Nom de la tâche
        4096,                   // Taille de la pile (en octets, souvent 4096 pour une tâche C++)
        reader,                 // Argument : Pointeur 'this' vers l'instance
        5, // Priorité (élevée)
        NULL                    // Handle de tâche (non utilisé ici)
    );

    }
    else{
        joysticksManager = new JoysticksManager(espNow);
        joysticksManager->initJoystick();
        buttonsManager = new ButtonsManager(espNow);
        buttonsManager->initButton();

        xTaskCreate([](void *)
                    { joysticksManager->Task(); },
                    "joystickManagerTask", 4096, &joysticksManager, 5, nullptr);

        xTaskCreate([](void *)
                    { buttonsManager->Task(); },
                    "buttonManagerTask", 4096, &buttonsManager, 5, nullptr);
    }
    
}
