#include "UsbManager.h"
#include "driver/uart.h"
#include <cstring>

static const int TX_PIN = UART_PIN_NO_CHANGE;
static const int RX_PIN = UART_PIN_NO_CHANGE;
static const int BUF_SIZE = 1024;
static const uart_port_t UART_NUM = UART_NUM_0;

void usb_comm_init() {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, nullptr, 0);
}

void usb_send_json(const std::string &json) {
    std::string msg = json + "\n";
    uart_write_bytes(UART_NUM, msg.c_str(), msg.length());
}

std::string usb_read_line() {
    uint8_t buf[BUF_SIZE];
    int len = uart_read_bytes(UART_NUM, buf, BUF_SIZE - 1, pdMS_TO_TICKS(10));
    if (len <= 0) {
        return "";
    }
    buf[len] = '\0';

    char *eol = (char*)std::strchr((char*)buf, '\n');
    if (eol) {
        *eol = '\0';
    }
    return std::string((char*)buf);
}

struct SimpleJson {
    std::string key;
    std::string value;
};

SimpleJson parse_simple_json(const std::string& s) {
    SimpleJson result;
    size_t p1 = s.find("\"");
    size_t p2 = s.find("\":\"");
    size_t p3 = s.rfind("\"");
    if (p1 != std::string::npos && p2 != std::string::npos && p3 != std::string::npos && p3 > p2 + 3) {
        result.key = s.substr(p1 + 1, p2 - (p1 + 1));
        result.value = s.substr(p2 + 3, p3 - (p2 + 3));
    }
    return result;
}


void UsbCommTask(void*) {
    usb_comm_init();
    usb_send_json("{\"status\":\"ready\"}");

    TickType_t last_heartbeat = xTaskGetTickCount();
    const TickType_t heartbeat_interval = pdMS_TO_TICKS(10000); 

    while (true) {
        TickType_t now = xTaskGetTickCount();

        // Toutes les 10 secondes, ça envoie un message pour s'assurer que la connexion est toujours là
        if ((now - last_heartbeat) >= heartbeat_interval) {
            usb_send_json("{\"Connection\":true}");
            last_heartbeat = now;
        }


        // Je suis parti du principe qu'on recevait et envoyait du json entre le téléphone et l'esp
        std::string line = usb_read_line();
        if (!line.empty() && line.find('{') != std::string::npos) {  
            auto parsed = parse_simple_json(line);
            if (!parsed.key.empty()) {
                usb_send_json("{\"received_key\":\"" + parsed.key + "\"}");
                usb_send_json("{\"received_value\":\"" + parsed.value + "\"}");
            } else {
                usb_send_json("{\"error\":\"invalid_json\"}");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}


void start_usb_comm_task() {
    xTaskCreate(UsbCommTask, "usbManagerTask", 4096, nullptr, 4, nullptr);
}