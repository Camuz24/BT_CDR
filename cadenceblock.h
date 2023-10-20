#ifndef CADENCEBLOCK_H
#define CADENCEBLOCK_H

#include "headers.h"

using namespace std;

class cadenceBlock
{
public:
    cadenceBlock();

    float computeCadence(float angle_old, float angle_now);

    float computeDelay(float cadence);

    double filterCadence(double cad_now, double filtC_old );



};

#endif // CADENCEBLOCK_H
