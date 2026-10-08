#include "../new_common.h"
#include "../driver/drv_spibus.h"

/* HSPI use */
#define OBK_USING_SPI2

void HAL_SPI_INIT(void);

obk_spibus_t *HAL_SPI_GETBUS(uint8_t bus_index);

obk_err_t HAL_SPI_CONFIGURE(obk_spidevice_t *device, obk_spi_config_t *configuration);

size_t HAL_SPI_XFER(obk_spidevice_t *device, obk_spi_message_t *message);