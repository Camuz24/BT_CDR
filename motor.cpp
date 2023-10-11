#include "motor.h"
#include<wiringPi.h>
#include<iostream>

motor::motor()
{

};

void motor::initMotor(){
    if (wiringPiSetup () == -1) exit (1) ;

    pinMode (PWM_pin, PWM_OUTPUT) ; /* set PWM pin as output */
    pwmSetClock(500);
    pwmSetRange(256);
    pinMode(enR,OUTPUT);
    pinMode(enL,OUTPUT);
    digitalWrite(enR,HIGH);
    digitalWrite(enL,HIGH);
    pwmWrite(PWM_pin,0);
}

void motor::setPwmMotor(int DutyCicle){
    if(DutyCicle>60) DutyCicle=60;
    if(DutyCicle<0) DutyCicle = 0;
    intensity = 255*DutyCicle/100;
    pwmWrite(PWM_pin,intensity);
}

void motor::stopMotor(){
    digitalWrite(enR,LOW);
    digitalWrite(enL,LOW);
    pwmWrite(PWM_pin,0);
}
