#ifndef CADENCEBLOCK_H
#define CADENCEBLOCK_H

#pragma once

class cadenceblock
{
public:
    cadenceblock();
    float computeCadence(float angle_old, float angle_now);
    double filterCadence(double cad_now, double filtC_old );

private:

};

#endif