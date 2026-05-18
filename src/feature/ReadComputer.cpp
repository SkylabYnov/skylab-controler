#include "ReadComputer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// --- Constructeur et Destructeur ---

ReadComputer::ReadComputer(EspNowHandler *espNowHandler, int baud_rate): espNowHandler(espNowHandler) {
    // 1. Allouer le buffer de lecture une seule fois
    this->rx_data_buffer = (uint8_t *) malloc(RD_BUF_SIZE + 1); // +1 pour le caractère nul
    if (!this->rx_data_buffer) {
        ESP_LOGE(TAG, "Échec de l'allocation du buffer RX.");
    }
    
    // 2. Initialisation des paramètres de l'UART (Configuration de base)
    const uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    // 3. Installation du pilote UART0 (TX/RX par défaut)
    // On utilise 0 pour l'UART_NUM_0, typiquement utilisé pour la console et les données.
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
    
    ESP_LOGI(TAG, "UART0 initialisé à %d bauds pour la communication PC.", baud_rate);
}

ReadComputer::~ReadComputer() {
    // Désinstaller le pilote et libérer le buffer
    if (this->rx_data_buffer) {
        free(this->rx_data_buffer);
    }
    uart_driver_delete(UART_NUM_0);
    ESP_LOGI(TAG, "UART0 désinstallé.");
}

// --- Fonction Statique pour xTaskCreate ---

void ReadComputer::Task(void* pvParameter) {
    // 1. Conversion du pointeur void* vers un pointeur de la classe ReadComputer
    ReadComputer* instance = static_cast<ReadComputer*>(pvParameter);
    
    // 2. Appel de la méthode d'instance qui contient la boucle de la tâche
    instance->task_loop();
    
    // Si la boucle se termine (ce qui ne devrait pas arriver dans une tâche FreeRTOS normale)
    vTaskDelete(NULL);
}

// --- Boucle de Tâche (Logique de Lecture) ---

void ReadComputer::task_loop() {
    // Récupérer le buffer pour plus de clarté    
    uint8_t byte;

    while (true) {

        // Sync header 0xAA 0x55
        uart_read_bytes(UART_NUM_0, &byte, 1, portMAX_DELAY);
        if (byte != 0xAA) continue;

        uart_read_bytes(UART_NUM_0, &byte, 1, portMAX_DELAY);
        if (byte != 0x55) continue;

        // Length
        uint8_t length;
        uart_read_bytes(UART_NUM_0, &length, 1, portMAX_DELAY);

        uint8_t payload[64];
        uart_read_bytes(UART_NUM_0, payload, length, portMAX_DELAY);

        uint8_t checksum;
        uart_read_bytes(UART_NUM_0, &checksum, 1, portMAX_DELAY);

        if (compute_checksum(payload, length) != checksum) {
            ESP_LOGW("RX", "Bad checksum");
            continue;
        }


        // Decode struct
        ControllerPacket *p = (ControllerPacket*)payload;

        int16_t newLeftX = (int16_t)(((p->LeftStickX + 1.0f) / 2.0f) * JoystickModel::JOYSTICK_MAX);
        int16_t newLeftY = (int16_t)(((p->LeftStickY + 1.0f) / 2.0f) * JoystickModel::JOYSTICK_MAX);
        JoystickModel left(newLeftX, newLeftY);

        int16_t newRightX = (int16_t)(((p->RightStickX + 1.0f) / 2.0f) * JoystickModel::JOYSTICK_MAX);
        int16_t newRightY = (int16_t)(((p->RightStickY + 1.0f) / 2.0f) * JoystickModel::JOYSTICK_MAX);
        JoystickModel right(newRightX, newRightY);
        
        if (lastLeft != left || lastRight != right)
        {
            ControllerRequestDTO dto;
            dto.ConvertJoyStickToFlightController(left, right);
            dto.initCounter();
            
            if(LimiteJoystick != -1){
                dto.flightController->throttle = dto.flightController->throttle * LimiteJoystick;
            }

            espNowHandler->send_data(dto);
            lastLeft = left;
            lastRight = right;
        }
        
        if (p->motorState != lastMotorState)
        {
            lastMotorState = p->motorState;
            ControllerRequestDTO controllerRequestDTO;
            controllerRequestDTO.buttonMotorState = new bool(p->motorState);
            controllerRequestDTO.initCounter();
            espNowHandler->send_data(controllerRequestDTO);
        }
        if (p->motorArming != lastMotorArming)
        {
            lastMotorArming = p->motorArming;
            ControllerRequestDTO controllerRequestDTO;
            controllerRequestDTO.buttonMotorArming = new bool(p->motorArming);
            controllerRequestDTO.initCounter();
            espNowHandler->send_data(controllerRequestDTO);
        }
    }
}

uint8_t ReadComputer::compute_checksum(const uint8_t *data, size_t len)
{
    uint32_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return sum & 0xFF; // garder seulement 1 octet
}
void ReadComputer::pushSample(JoystickModel *buf, int &sumX, int &sumY, const JoystickModel &sample)
{
    // Remove oldest
    sumX -= buf[idx].x;
    sumY -= buf[idx].y;

    // Insert new
    buf[idx] = sample;
    sumX += sample.x;
    sumY += sample.y;

    // Advance index
    idx = (idx + 1) % NBR_INCR_JOYSTICK;
}

JoystickModel ReadComputer::getAverage(int sumX, int sumY) const
{
    int avgX = sumX / NBR_INCR_JOYSTICK;
    int avgY = sumY / NBR_INCR_JOYSTICK;
    return {avgX, avgY};
}
