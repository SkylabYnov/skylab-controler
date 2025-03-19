#include "./JoysticksManager.h"
#include "JoysticksManager.h"


const char* JoysticksManager::Tag = "JoysticksManager";

JoysticksManager::JoysticksManager(UdpServer* udpServer)
    : udpServer(udpServer) {adc1_config_width(ADC_WIDTH_BIT_12);}

void JoysticksManager::Task() {
    while (true) {
        ControllerRequestDTO controllerRequestDTO;

        controllerRequestDTO.joystickLeft = new JoystickModel(adc1_get_raw(pinJoystickLeftX),adc1_get_raw(pinJoystickLeftY));
        controllerRequestDTO.joystickRight = new JoystickModel(adc1_get_raw(pinJoystickRightX),adc1_get_raw(pinJoystickRightY));

        
        addToLastRequests(lastControllerRequestDTO, NBR_INCR_JOKTICK, controllerRequestDTO);

        ControllerRequestDTO controllerRequestDTOAverage = calculateAverageDTO(lastControllerRequestDTO,NBR_INCR_JOKTICK);
        

        if(lastController!=controllerRequestDTOAverage){
            controllerRequestDTOAverage.initCounter();
            lastController = controllerRequestDTOAverage;
            cJSON* jsonObj = controllerRequestDTOAverage.toJson();
            char* jsonString = cJSON_PrintUnformatted(jsonObj);
            udpServer->SendMessage(jsonString);        
            delete jsonString;
            cJSON_Delete(jsonObj);
            
        }

        vTaskDelay(pdMS_TO_TICKS(TIME_MS_BETWEEN));
       
    }
}

void JoysticksManager::addToLastRequests(ControllerRequestDTO *list, int size, const ControllerRequestDTO &newRequest)
{
    // Décaler tous les éléments vers la gauche
    for (int i = 1; i < size; i++) {
        list[i - 1] = list[i];
    }

    // Ajouter le nouvel élément à la fin
    list[size - 1] = newRequest;
}

ControllerRequestDTO JoysticksManager::calculateAverageDTO(const ControllerRequestDTO *list, int size)
{
    if (size == 0) return {}; // Retourne un objet vide si la liste est vide

    double sumLeftX = 0.0, sumLeftY = 0.0;
    double sumRightX = 0.0, sumRightY = 0.0;
    int countLeft = 0, countRight = 0;

    for (int i = 0; i < size; i++) {
        if (list[i].joystickLeft) { // Vérifier si joystickLeft n'est pas null
            sumLeftX += list[i].joystickLeft->x;
            sumLeftY += list[i].joystickLeft->y;
            countLeft++;
        }

        if (list[i].joystickRight) { // Vérifier si joystickRight n'est pas null
            sumRightX += list[i].joystickRight->x;
            sumRightY += list[i].joystickRight->y;
            countRight++;
        }
    }

    // Éviter la division par zéro
    int avgLeftX = (countLeft > 0) ? sumLeftX / countLeft : 0;
    int avgLeftY = (countLeft > 0) ? sumLeftY / countLeft : 0;
    int avgRightX = (countRight > 0) ? sumRightX / countRight : 0;
    int avgRightY = (countRight > 0) ? sumRightY / countRight : 0;

    // Création du ControllerRequestDTO moyen
    ControllerRequestDTO averageDTO;
    averageDTO.joystickLeft = (countLeft > 0) ? new JoystickModel(avgLeftX, avgLeftY) : nullptr;
    averageDTO.joystickRight = (countRight > 0) ? new JoystickModel(avgRightX, avgRightY) : nullptr;

    return averageDTO;
}

void JoysticksManager::initJoystick()
{
    adc1_config_channel_atten(pinJoystickLeftX, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(pinJoystickLeftY, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(pinJoystickRightX, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(pinJoystickRightY, ADC_ATTEN_DB_11);
}
