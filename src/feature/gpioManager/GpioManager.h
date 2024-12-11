#ifndef GPIO_MANAGER_H
#define GPIO_MANAGER_H

#include "driver/gpio.h"
#include "./feature/udpServer/UdpServer.h"

class GpioManager {
public:
    GpioManager(gpio_num_t pin, UdpServer* udpServer);
    void Task();

private:
    gpio_num_t pin;
    UdpServer* udpServer;
    static const char *Tag;
};

#endif // GPIO_MANAGER_H
