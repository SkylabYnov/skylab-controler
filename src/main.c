#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"

// Configuration de la lampe
#define LAMP_PIN GPIO_NUM_2
#define BUTTON_PIN GPIO_NUM_15
#define BUF_SIZE 1024

void app_main() {
    gpio_set_direction(LAMP_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_level(LAMP_PIN, 0);

    const int uart_num = UART_NUM_0;
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(uart_num, &uart_config);
    uart_driver_install(uart_num, BUF_SIZE, 0, 0, NULL, 0);

    uint8_t data[BUF_SIZE];

    while (1) {
        int len = uart_read_bytes(uart_num, data, BUF_SIZE - 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            data[len] = '\0';

            if (strcmp((char *)data, "ON\n") == 0) {
                gpio_set_level(LAMP_PIN, 1);
            } else if (strcmp((char *)data, "OFF\n") == 0) {
                gpio_set_level(LAMP_PIN, 0);
            } else {
            }
        }

        if (gpio_get_level(BUTTON_PIN) == 0) {
            const char *message = "Button Pressed";
            uart_write_bytes(uart_num, message, strlen(message));

            while (gpio_get_level(BUTTON_PIN) == 1) {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }
    }
}