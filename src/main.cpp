#include "./feature/wifiServer/WifiServer.h"
#include "./feature/udpServer/UdpServer.h"
#include "./feature/gpioManager/GpioManager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"

UdpServer* udpServer;
GpioManager* gpioManager;

extern "C" void app_main() {
    
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    udpServer = new UdpServer(1234);
    gpioManager = new GpioManager(GPIO_NUM_26,udpServer);

    WifiServer wifiServer("ESP32_Hotspot", "12345678");
    wifiServer.Init();

    udpServer->Init();



    xTaskCreate([](void*) { udpServer->ReceiveTask(); },
                "gpioTask", 2048, &gpioManager, 5, nullptr);

    xTaskCreate([](void*) { gpioManager->Task(); },
                "gpioTask", 2048, &gpioManager, 5, nullptr);
}