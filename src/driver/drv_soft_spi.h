#ifndef __DRV_SOFT_SPI__
#define __DRV_SOFT_SPI__

#include "../new_common.h"

// PLEASE REMEMBER ABOUT THE CAPACITOR ON SCK!
// I had to add it for some FLASH memories, see:
// https://www.elektroda.com/rtvforum/viewtopic.php?p=20497046#20497046
typedef struct softSPI_s {
	byte sck;
	byte miso;
	byte mosi;
	byte ss;
} softSPI_t;

void SPI_Send(softSPI_t *spi, byte dataToSend);
byte SPI_Read(softSPI_t *spi);
void SPI_Begin(softSPI_t *spi);
void SPI_End(softSPI_t *spi);
void SPI_Setup(softSPI_t *spi);

#endif
