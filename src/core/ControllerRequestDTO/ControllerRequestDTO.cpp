#include "ControllerRequestDTO.h"
int ControllerRequestDTO::nmbInstanciation = 0;
ControllerRequestDTO::ControllerRequestDTO()
{
    counter = ControllerRequestDTO::nmbInstanciation++;
}
int ControllerRequestDTO::getCounter()
{
    return counter;
}
bool ControllerRequestDTO::operator==(const ControllerRequestDTO &other) const
{
    if (joystickLeft == other.joystickLeft && joystickRight == other.joystickRight)
    {
        return true;
    }
    return false;
}
std::string ControllerRequestDTO::toJson() const
{
    std::ostringstream oss;
    oss << "{\"joystickLeft\": " << joystickLeft.toJson()
        << ", \"joystickRight\": " << joystickRight.toJson()
        << ", \"counter\": " << counter
        << "}";
    return oss.str();
}
ControllerRequestDTO ControllerRequestDTO::fromJson(const std::string &jsonString)
{
    ControllerRequestDTO dto;
    size_t posJoystickLeft = jsonString.find("\"joystickLeft\":");
    size_t posJoystickRight = jsonString.find("\"joystickRight\":");
    size_t posCounter = jsonString.find("\"counter\":");
    size_t posNmbInstanciation = jsonString.find("\"nmbInstanciation\":");

    if (posJoystickLeft == std::string::npos || posJoystickRight == std::string::npos ||
        posCounter == std::string::npos || posNmbInstanciation == std::string::npos)
    {
    }

    std::string joystickLeftJson = jsonString.substr(
        posJoystickLeft + 15, jsonString.find('}', posJoystickLeft) - (posJoystickLeft + 14));
    std::string joystickRightJson = jsonString.substr(
        posJoystickRight + 16, jsonString.find('}', posJoystickRight) - (posJoystickRight + 15));

    dto.joystickLeft = JoystickModel::fromJson(joystickLeftJson);
    dto.joystickRight = JoystickModel::fromJson(joystickRightJson);

    size_t counterEnd = jsonString.find(',', posCounter);
    dto.counter = std::stoi(jsonString.substr(posCounter + 10, counterEnd - (posCounter + 10)));

    size_t nmbInstanciationEnd = jsonString.find('}', posNmbInstanciation);
    nmbInstanciation = std::stoi(jsonString.substr(posNmbInstanciation + 19, nmbInstanciationEnd - (posNmbInstanciation + 19)));

    return dto;
};