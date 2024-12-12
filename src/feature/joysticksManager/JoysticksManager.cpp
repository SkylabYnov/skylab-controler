#include "./JoysticksManager.h"

const char* JoysticksManager::Tag = "JoysticksManager";

JoysticksManager::JoysticksManager(UdpServer* udpServer)
    : udpServer(udpServer) {adc1_config_width(ADC_WIDTH_BIT_12);}

void JoysticksManager::Task() {
    while (true) {
       JoystickModel* joystick = new JoystickModel( adc1_get_raw(pinJoystickX),
                                                    adc1_get_raw(pinJoystickY));
        if(oldJoystickGauche==nullptr){
            oldJoystickGauche= new JoystickModel(joystick);
        }

        if(oldJoystickGauche!=joystick){
            oldJoystickGauche= new JoystickModel(joystick);
        }
       vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void JoysticksManager::initJoystick()
{
    adc1_config_channel_atten(pinJoystickX, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(pinJoystickY, ADC_ATTEN_DB_11);

}
