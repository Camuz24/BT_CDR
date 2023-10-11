#include "cadenceblock.h"
#include "headers.h"

using namespace std;

cadenceblock::cadenceblock()
{

}

long cadenceblock::timeStructToMs(timespec t) // transforms time as struct to a long integer (time in ms)
{
    long result;

    result = t.tv_sec*1000 + t.tv_nsec/1000000;

    return result;
}

float cadenceblock::computeCadence(float previous_angle, float now_angle)
{
    float cadence;

    if(previous_angle == -1) { // FIRST ITERATION
        clock_gettime (CLOCK_MONOTONIC, &tau_old);
        angle_old = now_angle;
    }

    else {
        //previous_angle_difference = angle_difference;
        if(now_angle > angle_old){  // normal case
            angle_difference = now_angle - angle_old;}

        else if(now_angle==angle_old) 
        {
            angle_difference = 0;
            cadence = 0;
        }

        else if(now_angle<angle_old){// in case difference is negative, meaning we are passing from 360 to 0
            angle_difference = (now_angle + 360) - angle_old;}
       
        if(angle_difference >= interval) { // Compute cadence
            clock_gettime (CLOCK_MONOTONIC, &tau_now);
            //cadence = ((angle_difference/(timeStructToMs(tau_now) - timeStructToMs(tau_old))) * 60)/360.0;
            cadence = ((angle_difference/(timeStructToMs(tau_now) - timeStructToMs(tau_old))) * 1000 * 60)/360.0;
            previous_cadence = cadence;
            tau_old = tau_now;
            angle_old = now_angle;
        }

        else if(angle_difference < interval && angle_difference != 0){
            cadence = previous_cadence;}
    }

    return cadence;
}

double cadenceblock::filterCadence(double cad_now, double filtC_old )
{
        double filtC_now = 0.0;
        //float absDiff = abs (cad_now-filtC_old); // to choose different value of lambda
        double lambda=0.1; // default value

        //TO DO: adding different value of lambda

        filtC_now = lambda*cad_now + ( 1 - lambda)*filtC_old;
        return (filtC_now);
}