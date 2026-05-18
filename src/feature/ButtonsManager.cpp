#include "ButtonsManager.h"
#include "esp_log.h"
#include <esp_timer.h>

#define TAG "ButtonsManager"

ButtonsManager::ButtonsManager(EspNowHandler *espNowHandler) : espNowHandler(espNowHandler)
{
}

void ButtonsManager::initButton()
{
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << pinButtonArming) | (1ULL << pinButtonMotorState);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(pinButtonArming, button_isr_handler_arming, this);
    gpio_isr_handler_add(pinButtonMotorState, button_isr_handler_motor, this);

    // Create debounce timers
    esp_timer_create_args_t timer_args_arming = {
        .callback = &ButtonsManager::debounce_timer_callback_arming,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "debounce_arming"
    };
    esp_timer_create(&timer_args_arming, &debounce_timer_arming);

    esp_timer_create_args_t timer_args_motor = {
        .callback = &ButtonsManager::debounce_timer_callback_motor,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "debounce_motor"
    };
    esp_timer_create(&timer_args_motor, &debounce_timer_motor);
}

void ButtonsManager::Task()
{
    while (true)
    {
        if (buttonPressedMotorState)
        {
            buttonPressedMotorStateValue = !buttonPressedMotorStateValue;
            ControllerRequestDTO controllerRequestDTO;
            controllerRequestDTO.buttonMotorState = new bool(buttonPressedMotorStateValue);
            controllerRequestDTO.initCounter();
            buttonPressedMotorState = false;
            espNowHandler->send_data(controllerRequestDTO);
            ESP_LOGI(TAG, "Button Motor State Pressed. New State: %s", buttonPressedMotorStateValue ? "ON" : "OFF");
        }
        if (buttonPressedMotorArming)
        {
            buttonPressedMotorArmingValue = !buttonPressedMotorArmingValue;
            ControllerRequestDTO controllerRequestDTO;
            controllerRequestDTO.buttonMotorArming = new bool(buttonPressedMotorArmingValue);
            controllerRequestDTO.initCounter();
            buttonPressedMotorArming = false;
            ESP_LOGI(TAG, "Button Motor Arming Pressed. New State: %s", buttonPressedMotorArmingValue ? "ARMED" : "DISARMED");
            espNowHandler->send_data(controllerRequestDTO);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void IRAM_ATTR ButtonsManager::button_isr_handler_arming(void *arg)
{
    ButtonsManager *self = static_cast<ButtonsManager *>(arg);
    gpio_intr_disable(self->pinButtonArming);
    esp_timer_start_once(self->debounce_timer_arming, 100000);
    self->buttonPressedMotorArming = true;
}
void IRAM_ATTR ButtonsManager::button_isr_handler_motor(void *arg)
{
    ButtonsManager *self = static_cast<ButtonsManager *>(arg);
    gpio_intr_disable(self->pinButtonMotorState);
    esp_timer_start_once(self->debounce_timer_motor, 100000);
    self->buttonPressedMotorState = true;
}

void ButtonsManager::debounce_timer_callback_arming(void *arg)
{
    ButtonsManager *self = static_cast<ButtonsManager *>(arg);
    gpio_intr_enable(self->pinButtonArming);
}

void ButtonsManager::debounce_timer_callback_motor(void *arg)
{
    ButtonsManager *self = static_cast<ButtonsManager *>(arg);
    gpio_intr_enable(self->pinButtonMotorState);
}
