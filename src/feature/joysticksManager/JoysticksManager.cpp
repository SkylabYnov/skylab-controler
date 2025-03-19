#include "./JoysticksManager.h"
#include "JoysticksManager.h"



const char* JoysticksManager::Tag = "JoysticksManager";

JoysticksManager::JoysticksManager(UdpServer* udpServer)
    : udpServer(udpServer) {adc1_config_width(ADC_WIDTH_BIT_12);}

void JoysticksManager::Task() {
    while (true) {
        JoystickModel joystickLeft = JoystickModel(adc1_get_raw(pinJoystickLeftX),adc1_get_raw(pinJoystickLeftY));
        JoystickModel joystickRight = JoystickModel(adc1_get_raw(pinJoystickRightX),adc1_get_raw(pinJoystickRightY));

        
        addToLastRequests(lastJoystickModelLeftTable, NBR_INCR_JOKTICK, joystickLeft);
        addToLastRequests(lastJoystickModelRightTable, NBR_INCR_JOKTICK, joystickRight);

        JoystickModel joystickModelLeftAverage = calculateAverageDTO(lastJoystickModelLeftTable,NBR_INCR_JOKTICK);
        JoystickModel joystickModelRightAverage = calculateAverageDTO(lastJoystickModelRightTable,NBR_INCR_JOKTICK);
        
        if(lastJoystickModelLeft!=joystickModelLeftAverage || lastJoystickModelRight!=joystickModelRightAverage){
            ControllerRequestDTO controllerRequestDTO;
            controllerRequestDTO.ConvertJoyStickToFlightController(joystickModelLeftAverage,joystickModelRightAverage);
            controllerRequestDTO.initCounter();
            lastJoystickModelLeft = joystickModelLeftAverage;
            lastJoystickModelRight = joystickModelRightAverage;
            cJSON* jsonObj = controllerRequestDTO.toJson();
            char* jsonString = cJSON_PrintUnformatted(jsonObj);
            udpServer->SendMessage(jsonString);        
            delete jsonString;
            cJSON_Delete(jsonObj);
        }

        vTaskDelay(pdMS_TO_TICKS(TIME_MS_BETWEEN));
       
    }
}

void JoysticksManager::addToLastRequests(JoystickModel *list, int size, const JoystickModel &newRequest)
{
    // Décaler tous les éléments vers la gauche
    for (int i = 1; i < size; i++) {
        list[i - 1] = list[i];
    }

    // Ajouter le nouvel élément à la fin
    list[size - 1] = newRequest;
}

JoystickModel JoysticksManager::calculateAverageDTO(const JoystickModel *list, int size) {
    if (size == 0) return {}; // Retourne un objet vide si la liste est vide

    double sumX = 0.0, sumY = 0.0;
    int count = 0;

    for (int i = 0; i < size; i++) {
        sumX += list[i].x;
        sumY += list[i].y;
        count++;
    }

    // Éviter la division par zéro
    int avgX = (count > 0) ? static_cast<int>(sumX / count) : 0;
    int avgY = (count > 0) ? static_cast<int>(sumY / count) : 0;

    return JoystickModel(avgX, avgY);
}


void JoysticksManager::initJoystick()
{
    adc1_config_channel_atten(pinJoystickLeftX, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(pinJoystickLeftY, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(pinJoystickRightX, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(pinJoystickRightY, ADC_ATTEN_DB_11);
}
