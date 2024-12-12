#ifndef Joystick_Model_H
#define Joystick_Model_H

#include <string>
#include <sstream>
#include <iostream>

class JoystickModel{
    public:
        int x;
        int y;
        JoystickModel(int x, int y);
        JoystickModel(JoystickModel* joystickModel);
        JoystickModel();
        bool operator==(const JoystickModel& )const;
        std::string toJson() const;
        static JoystickModel fromJson(const std::string& jsonString);
    private:
        int roomForManeuver = 50;
};

#endif