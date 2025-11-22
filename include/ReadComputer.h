#ifndef READ_COMPUTER_HPP
#define READ_COMPUTER_HPP

#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>
#include "EspNowHandler.h"

struct ControllerPacket {
    float RightStickY;
    float RightStickX;
    float LeftStickY;
    float LeftStickX;
    uint8_t motorState;
    uint8_t motorArming;
};

class ReadComputer {
private:
    EspNowHandler *espNowHandler;
    const int BUF_SIZE = 1024;
    const int RD_BUF_SIZE = 1024;

    JoystickModel lastLeft;
    JoystickModel lastRight;

    bool lastMotorArming = false;
    bool lastMotorState = false;
    
    // Pointeur pour le buffer de lecture
    uint8_t *rx_data_buffer; 

    /**
     * @brief Méthode non statique contenant la logique de lecture série.
     */
    void task_loop();
    uint8_t compute_checksum(const uint8_t* data, size_t len);

public:
    /**
     * @brief Constructeur de la classe ReadComputer.
     */
    ReadComputer(EspNowHandler *espNowHandler, int baud_rate = 115200);

    /**
     * @brief Destructeur. Nettoie les ressources.
     */
    ~ReadComputer();
    
    /**
     * @brief Wrapper statique pour xTaskCreate.
     * @param pvParameter Pointeur vers l'instance de la classe (this).
     */
    static void Task(void* pvParameter);

    /**
     * @brief Initialise le pilote UART pour la réception des données.
     */
    void init();
};

#endif // READ_COMPUTER_HPP