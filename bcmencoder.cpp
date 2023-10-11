#include "bcmencoder.h"

BcmEncoder::BcmEncoder()
{

}

bool BcmEncoder::initEncoder(){
    if (!bcm2835_init())
    {
      printf("bcm2835_init failed. Are you running as root??\n");
      return 0;
    }


    if (!bcm2835_spi_begin())
    {
      printf("bcm2835_spi_begin failed. Are you running as root??\n");
      return 0;
    }

    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE1);
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_256); //256
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0,LOW);


    //std::cout << "init ok\n";

    memcpy(data, &addr, 2);

    return 1;

}

float BcmEncoder::getAngle(){
    bcm2835_spi_transfernb((char*) tx_buf, (char*) rx_buf, sizeof(tx_buf));
    //for( size_t i = 0; i<2; i++)
        //std::cout << std::hex << int(tx_buf[i]) << std::endl;

    bcm2835_spi_transfernb((char*) tx_buf, (char*) rx_buf, sizeof(tx_buf));
    //for( size_t i = 0; i<2; i++)
        //std::cout << std::hex << int(rx_buf[i]) << std::endl;

    pos = rx_buf[0] << 8 | rx_buf[1];
    pos &= 0x3fff;
    //printf("\n %x,%x \n", rx_buf[0], rx_buf[1]);
    //std::cout << "pos = " << pos << std::endl;
    pos_deg = pos * (360.0f/16384.0f);
    //std::cout << "pos = " << pos_deg << std::endl;

    //std::cout << "-------\n";
    //sleep(1);
    // run = false;

    return pos_deg;

}
