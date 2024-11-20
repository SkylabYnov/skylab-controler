#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/gpio.h"

// Configuration Wi-Fi
#define WIFI_SSID      "ESP32_Hotspot"
#define WIFI_PASSWORD  "12345678"
#define UDP_PORT       1234
#define BUTTON_PIN     26

static const char *TAG = "ESP32_UDP";
char ipDrone[INET_ADDRSTRLEN] = "";
int sock;

// Initialisation du point d'accès Wi-Fi
void wifi_init_softap() {
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .channel = 1,
            .password = WIFI_PASSWORD,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };
    if (strlen(WIFI_PASSWORD) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Point d'accès initialisé avec SSID: %s", WIFI_SSID);
}

// Configuration du socket UDP
void my_udp_init() {
    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Erreur de création du socket : errno %d", errno);
        return;
    }

    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(UDP_PORT);

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Erreur lors du bind : errno %d", errno);
        close(sock);
        return;
    }

    ESP_LOGI(TAG, "Serveur UDP prêt sur le port %d", UDP_PORT);
}

// Tâche pour gérer les messages UDP entrants
void udp_receive_task(void *pvParameters) {
    char rx_buffer[128];
    struct sockaddr_in source_addr;
    socklen_t socklen = sizeof(source_addr);

    while (1) {
        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
        if (len < 0) {
            ESP_LOGE(TAG, "Erreur de réception : errno %d", errno);
            break;
        } else {
            rx_buffer[len] = '\0'; // Terminer la chaîne
            ESP_LOGI(TAG, "Message reçu : %s", rx_buffer);

            // Si le message est "hello world !", enregistrer l'adresse IP du drone
            if (strcmp(rx_buffer, "hello world !") == 0) {
                inet_ntop(AF_INET, &source_addr.sin_addr, ipDrone, INET_ADDRSTRLEN);
                ESP_LOGI(TAG, "IP Drone enregistrée : %s", ipDrone);
            }
        }
    }
    vTaskDelete(NULL);
}

// Tâche pour gérer le bouton et envoyer des messages
void button_task(void *pvParameters) {
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY);

    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(UDP_PORT);

    while (1) {
        if (gpio_get_level(BUTTON_PIN) == 0) {
            if (strlen(ipDrone) > 0) {
                inet_pton(AF_INET, ipDrone, &dest_addr.sin_addr.s_addr);
                const char *message = "l1";
                sendto(sock, message, strlen(message), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
                ESP_LOGI(TAG, "Message envoyé : %s", message);
            } else {
                ESP_LOGW(TAG, "Aucune IP Drone enregistrée !");
            }
            vTaskDelay(pdMS_TO_TICKS(500)); // Anti-rebond
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Fonction principale
void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_softap();
    my_udp_init();

    // Démarrer les tâches FreeRTOS
    xTaskCreate(udp_receive_task, "udp_receive_task", 4096, NULL, 5, NULL);
    xTaskCreate(button_task, "button_task", 2048, NULL, 5, NULL);
}
