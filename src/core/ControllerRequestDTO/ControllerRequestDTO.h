#ifndef Controller_Request_DTO_H
#define Controller_Request_DTO_H
#include <core/JoystickModel/JoystickModel.h>
#include <esp_log.h>

class ControllerRequestDTO {
    public:
        JoystickModel joystickLeft;
        JoystickModel joystickRight;
        ControllerRequestDTO();
        int getCounter();
        bool operator==(const ControllerRequestDTO& )const;
        std::string toJson() const;
        static ControllerRequestDTO fromJson(const std::string& jsonString);
    private:
        static int nmbInstanciation;
        int counter = 0;
};
#endif