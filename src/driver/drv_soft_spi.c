#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_soft_spi.h"
#include "../httpserver/new_http.h"
#include "../hal/hal_pins.h"

#define MY_SPI_DELAY usleep(500);

void SPI_Send(softSPI_t *spi, byte dataToSend) {
	for (int i = 0; i < 8; i++) {
		MY_SPI_DELAY;
		HAL_PIN_SetOutputValue(spi->mosi, (dataToSend >> (7 - i)) & 0x01);
		MY_SPI_DELAY;
		HAL_PIN_SetOutputValue(spi->sck, 1);
		MY_SPI_DELAY;
		HAL_PIN_SetOutputValue(spi->sck, 0);
	}
}

byte SPI_Read(softSPI_t *spi) {
	byte receivedData = 0;
	for (int i = 0; i < 8; i++) {
		MY_SPI_DELAY;
		HAL_PIN_SetOutputValue(spi->sck, 1);
		MY_SPI_DELAY;
		receivedData |= (HAL_PIN_ReadDigitalInput(spi->miso) << (7 - i));
		MY_SPI_DELAY;
		HAL_PIN_SetOutputValue(spi->sck, 0);
	}
	return receivedData;
}

void SPI_Begin(softSPI_t *spi) {
	HAL_PIN_SetOutputValue(spi->ss, 0); // enable SPI communication with the flash
}
void SPI_End(softSPI_t *spi) {
	HAL_PIN_SetOutputValue(spi->ss, 1); // disable SPI communication with the flash
}
void SPI_Setup(softSPI_t *spi) {
	//if (spi->spi_Ready)
	//	return;
	//spi->spi_Ready = 1;
	HAL_PIN_Setup_Output(spi->sck);
	HAL_PIN_Setup_Input(spi->miso);
	HAL_PIN_Setup_Output(spi->mosi);
	HAL_PIN_Setup_Output(spi->ss);
	HAL_PIN_SetOutputValue(spi->ss, 1); // set SS_PIN to inactive
}


