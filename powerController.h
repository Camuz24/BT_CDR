#ifndef POWERCONTROLLER_H
#define POWERCONTROLLER_H

#include "headers.h"
#pragma once

class powerController
{
public:
    powerController();
    ~powerController();
    void PidON();
    void PidOFF();
    float PID(float target_power, float average_power);

private:

        bool powerPidEnabled = false;

        double max = 1;
        double min = -1;
        double error = 0;
        double integral = 0;
        double oldError = 0;

        double output = 0;

        double kp = 0.002;
        double ki = 0.000025;
        double kd = 0.0;

};

#endif