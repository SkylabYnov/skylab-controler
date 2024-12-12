class JoystickModel{
    public:
        int x;
        int y;
        JoystickModel(int x, int y);
        JoystickModel(JoystickModel* joystickModel);
        bool operator==(const JoystickModel& )const;
    private:
        int roomForManeuver = 50;
};