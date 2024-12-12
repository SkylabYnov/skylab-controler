#include "JoystickModel.h"

JoystickModel::JoystickModel(int x, int y)
:x(x),y(y){}

JoystickModel::JoystickModel(JoystickModel *joystickModel)
{
    x=joystickModel->x;
    y=joystickModel->y;
}
bool JoystickModel::operator==(const JoystickModel &other) const
{
    if( x>=other.x-roomForManeuver && x<=other.x+roomForManeuver &&
        y>=other.y-roomForManeuver && y<=other.y+roomForManeuver )
    {
        return true;
    }
    return false;


};