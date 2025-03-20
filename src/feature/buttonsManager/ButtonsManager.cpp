#include "./ButtonsManager.h"
#include "ButtonsManager.h"
#include "esp_log.h"


#define TAG "ButtonsManager"

ButtonsManager::ButtonsManager(EspNowHandler* espNowHandler): espNowHandler(espNowHandler)  {
}


void ButtonsManager::initButton()
{   
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << pinButtonEmergencyStop) | (1ULL << pinButtonMotorState);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_NEGEDGE; 
    gpio_config(&io_conf);

    gpio_install_isr_service(0); 
    gpio_isr_handler_add(pinButtonEmergencyStop, button_isr_handler_emergency, this);
    gpio_isr_handler_add(pinButtonMotorState, button_isr_handler_motor, this);
}


void ButtonsManager::Task()
{
    while (true) {
        if (buttonPressedMotorState) {
            ControllerRequestDTO controllerRequestDTO;
            controllerRequestDTO.buttonMotorState=new bool(true);
            controllerRequestDTO.initCounter();
            buttonPressedMotorState = false; 
            espNowHandler->send_data(controllerRequestDTO);
            espNowHandler->send_data(controllerRequestDTO);
        }
        if (buttonPressedEmergencyStop) {
            ControllerRequestDTO controllerRequestDTO;
            controllerRequestDTO.buttonEmergencyStop=new bool(true);
            controllerRequestDTO.initCounter();
            buttonPressedEmergencyStop = false; 
            espNowHandler->send_data(controllerRequestDTO);
            espNowHandler->send_data(controllerRequestDTO);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void IRAM_ATTR ButtonsManager::button_isr_handler_emergency(void *arg)
{
    ButtonsManager* self = static_cast<ButtonsManager*>(arg);
    self->buttonPressedEmergencyStop = true;
}
void IRAM_ATTR ButtonsManager::button_isr_handler_motor(void *arg)
{
    ButtonsManager* self = static_cast<ButtonsManager*>(arg);
    self->buttonPressedMotorState = true;
}
