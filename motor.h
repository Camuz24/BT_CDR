#ifndef MOTOR_H
#define MOTOR_H

#include "headers.h"
class motor
{
private:
    const int PWM_pin = 26;
    const int enR = 27;
    const int enL = 28;
public:
    motor();
    void initMotor();
    void setPwmMotor(int DutyCicle);
    void stopMotor();
    int intensity;

};

#endif // MOTOR_H
