#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/ledc.h"
#include "esp_log.h"

#define TAG "RGB_CONTROLLER"

// Configuration UART
#define UART_NUM UART_NUM_0
#define BUF_SIZE 1024

// Configuration des broches RGB
#define RED_PIN GPIO_NUM_33
#define GREEN_PIN GPIO_NUM_25
#define BLUE_PIN GPIO_NUM_32

// Configuration LED PWM
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL_R LEDC_CHANNEL_0
#define LEDC_CHANNEL_G LEDC_CHANNEL_1
#define LEDC_CHANNEL_B LEDC_CHANNEL_2
#define LEDC_DUTY_RES LEDC_TIMER_8_BIT
#define LEDC_FREQUENCY 5000 // 5 kHz

void configure_ledc_channel(ledc_channel_t channel, gpio_num_t pin) {
    ledc_channel_config_t ledc_channel = {
        .channel    = channel,
        .duty       = 0,
        .gpio_num   = pin,
        .speed_mode = LEDC_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_TIMER
    };
    ledc_channel_config(&ledc_channel);
}

void configure_led_pwm() {
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    configure_ledc_channel(LEDC_CHANNEL_R, RED_PIN);
    configure_ledc_channel(LEDC_CHANNEL_G, GREEN_PIN);
    configure_ledc_channel(LEDC_CHANNEL_B, BLUE_PIN);
}

void set_rgb_color(uint8_t red, uint8_t green, uint8_t blue) {
    ESP_LOGI(TAG, "Setting duty cycle: R=%d, G=%d, B=%d", red, green, blue);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_R, red);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_R);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_G, green);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_G);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_B, blue);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_B);
}

void uart_task(void *arg) {
    uint8_t data[BUF_SIZE];
    uint8_t red, green, blue;

    while (1) {
        int len = uart_read_bytes(UART_NUM, data, BUF_SIZE - 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            data[len] = '\0'; // Terminer la chaîne
            ESP_LOGI(TAG, "Received: %s", data);

            uint8_t new_red, new_green, new_blue;
            if (sscanf((char *)data, "%hhu,%hhu,%hhu", &new_red, &new_green, &new_blue) == 3) {
                red = new_red;
                green = new_green;
                blue = new_blue;
                ESP_LOGI(TAG, "Setting RGB to R=%d, G=%d, B=%d", red, green, blue);
                set_rgb_color(red, green, blue);
            } else {
                ESP_LOGW(TAG, "Invalid RGB format");
            }
        }
    }
}

void app_main() {
    // Configuration des GPIO et PWM pour la LED RGB
    configure_led_pwm();

    // Configuration UART
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_NUM, &uart_config);
    uart_driver_install(UART_NUM, BUF_SIZE, 0, 0, NULL, 0);

    // Lancer la tâche UART
    xTaskCreate(uart_task, "uart_task", 2048, NULL, 10, NULL);
}