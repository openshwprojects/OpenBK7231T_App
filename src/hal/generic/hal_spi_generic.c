#include "../hal_spi.h"

void __attribute__((weak)) HAL_SPI_INIT(void) {
	
}

obk_spibus_t * __attribute__((weak)) HAL_SPI_GETBUS(uint8_t bus_index) {
	return 0;
}
/**
 * @brief Configure HAL SPI
 */ 
obk_err_t __attribute__((weak)) HAL_SPI_CONFIGURE(obk_spidevice_t *device, obk_spi_config_t *configuration) {
	return 0;
}
/**
 * @brief Transfer message HAL SPI
 */ 
size_t __attribute__((weak)) HAL_SPI_XFER(obk_spidevice_t *device, obk_spi_message_t *message) {
	return 0;
}