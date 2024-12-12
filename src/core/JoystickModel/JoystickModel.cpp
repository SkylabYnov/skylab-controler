#include "JoystickModel.h"

JoystickModel::JoystickModel(int x, int y)
    : x(x), y(y) {}

JoystickModel::JoystickModel(JoystickModel *joystickModel)
{
    if (joystickModel)
    {
        x = joystickModel->x;
        y = joystickModel->y;
    }
    else
    {
        x = 0;
        y = 0;
    }
}
bool JoystickModel::operator==(const JoystickModel &other) const
{
    if (x >= other.x - roomForManeuver && x <= other.x + roomForManeuver &&
        y >= other.y - roomForManeuver && y <= other.y + roomForManeuver)
    {
        return true;
    }
    return false;
}
JoystickModel::JoystickModel() : x(0), y(0) {};
std::string JoystickModel::toJson() const
{
    std::ostringstream oss;
    oss << "{\"x\": " << x << ", \"y\": " << y << "}";
    return oss.str();
}
JoystickModel JoystickModel::fromJson(const std::string &jsonString)
{
    int x = 0, y = 0;
    size_t posX = jsonString.find("\"x\":");
    size_t posY = jsonString.find("\"y\":");

    if (posX == std::string::npos || posY == std::string::npos)
    {
    }

    posX += 4; // Avancer après "x":
    posY += 4; // Avancer après "y":

    x = std::stoi(jsonString.substr(posX, jsonString.find(',', posX) - posX));
    y = std::stoi(jsonString.substr(posY, jsonString.find('}', posY) - posY));

    return JoystickModel(x, y);
};