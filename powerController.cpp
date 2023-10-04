#include "powerController.h"

powerController::powerController()
{

}

powerController::~powerController()
{
    
}

float powerController::PID(float target_power, float average_power)
{
    if(!powerPidEnabled) output = 0;
    else{
        error = target_power - average_power;
        integral += error;
        output = kp*error + ki*integral + kd*(error - oldError);
        if(output < min) output = min;
        if(output > max) output = max;
        oldError = error;
    }
    return output;
}

void powerController::PidON()
{
    powerPidEnabled = true; 
}

void powerController::PidOFF()
{
    powerPidEnabled = false; 
}