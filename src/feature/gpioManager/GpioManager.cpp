#include "GpioManager.h"

const char* GpioManager::Tag = "GpioManager";

GpioManager::GpioManager(gpio_num_t pin, UdpServer* udpServer)
    : pin(pin), udpServer(udpServer) {
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
}

void GpioManager::Task() {
    while (true) {
        if (gpio_get_level(pin) == 0) {
            udpServer->SendMessage("REPOND STP OH MAXENCE STP ALLEZ REPOND MAIS MERDE ALLELLEZZZZZZ TU GAVES ALLUME LA LAMPE BORDEL", "192.168.4.2");
            vTaskDelay(pdMS_TO_TICKS(500)); // Anti-rebond
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
