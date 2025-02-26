#include "./JoysticksManager.h"


const char* JoysticksManager::Tag = "JoysticksManager";

JoysticksManager::JoysticksManager(UdpServer* udpServer)
    : udpServer(udpServer) {adc1_config_width(ADC_WIDTH_BIT_12);}

void JoysticksManager::Task() {
    while (true) {
        ControllerRequestDTO controllerRequestDTO;

        controllerRequestDTO.joystickLeft = new JoystickModel( adc1_get_raw(pinJoystickX),
                                                    adc1_get_raw(pinJoystickY));

                                                    
        controllerRequestDTO.joystickRight = new JoystickModel( adc1_get_raw(pinJoystick2X),
        adc1_get_raw(pinJoystick2Y));


        if(lastController!=controllerRequestDTO){
            lastController = controllerRequestDTO;
            cJSON* jsonObj = controllerRequestDTO.toJson();
            char* jsonString = cJSON_PrintUnformatted(jsonObj);
            udpServer->SendMessage(jsonString);        
            delete jsonString;
            cJSON_Delete(jsonObj);
            
        }


       vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void JoysticksManager::initJoystick()
{
    adc1_config_channel_atten(pinJoystickX, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(pinJoystickY, ADC_ATTEN_DB_11);

}
