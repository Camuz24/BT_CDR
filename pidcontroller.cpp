#include "pidcontroller.h"

pidController::pidController()
{

}
double pidController::PID(double target, double cadence){
    if(pidEnable==1){
    error=target-cadence;
    integral += error;
    output= kp*error + ki*integral + kd*(error-oldError);
    if(output<min) output= min;
    if(output>max) output= max;
    oldError=error;
    } else output=0;
return output;
}
void pidController::off(){
    pidEnable=0;
}
void pidController::on(){
    pidEnable=1;
}
