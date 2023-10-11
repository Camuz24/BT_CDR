#ifndef CADENCEBLOCK_H
#define CADENCEBLOCK_H
#include "headers.h"

#pragma once

class cadenceblock
{
public:
    cadenceblock();
    float computeCadence(float angle_old, float angle_now);
    double filterCadence(double cad_now, double filtC_old );
    long timeStructToMs(timespec t);

private:
    float angle_old;
    float angle_difference = -99.0;
    float previous_angle_difference = -99.0;
    int interval = 30;
    struct timespec tau_now;
    struct timespec tau_old;
    float previous_cadence = 0;

};

#endif