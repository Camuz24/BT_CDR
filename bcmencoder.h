#ifndef BCMENCODER_H
#define BCMENCODER_H
#include <bcm2835.h>
#include <iostream>
#include <unistd.h>
#include <string.h>


class BcmEncoder
{

public:
    uint8_t data[2] = {0x00}; // = {0x3f, 0xff, 0x00, 0x00};
    uint16_t addr = 0x0001 | 0x4000;
    uint8_t tx_buf[2] = {0x3f, 0xff};
    uint8_t rx_buf[2] = {0x00};
    uint16_t pos;
    float pos_deg = 0.0f;

    BcmEncoder();
    bool initEncoder();
    float getAngle();


};

#endif // BCMENCODER_H
