#if PLATFORM_BK7231N
#include "../../beken378/func/user_driver/armino/spi/spi.h"
#else
// spi_config_t and its member types are copied from BK7321N SPI implementation.
#include "drv_spi.h"
#endif

#include "../logging/logging.h"

int SPI_DriverInit(void) {
#if PLATFORM_BK7231N
    return bk_spi_driver_init();
#else
    ADDLOG_ERROR(LOG_FEATURE_DRV, "SPI_DriverInit not supported");
    return -1;
#endif
}

int SPI_DriverDeinit(void) {
#if PLATFORM_BK7231N
    return bk_spi_driver_deinit();
#else
    ADDLOG_ERROR(LOG_FEATURE_DRV, "SPI_DriverDeinit not supported");
    return -1;
#endif
}

int SPI_Init(const spi_config_t *config) {
#if PLATFORM_BK7231N
	return bk_spi_init(0, config);
#else
    ADDLOG_ERROR(LOG_FEATURE_DRV, "SPI_Init not supported");
    return -1;
#endif
}

int SPI_Deinit(void) {
    #if PLATFORM_BK7231N
	return bk_spi_deinit(0);
#else
    ADDLOG_ERROR(LOG_FEATURE_DRV, "SPI_Deinit not supported");
    return -1;
#endif
}

int SPI_WriteBytes(const void *data, uint32_t size) {
    #if PLATFORM_BK7231N
	return bk_spi_write_bytes(0, data, size);
#else
    ADDLOG_ERROR(LOG_FEATURE_DRV, "SPI_WriteBytes not supported");
    return -1;
#endif
}

int SPI_ReadBytes(void *data, uint32_t size) {
    #if PLATFORM_BK7231N
	return bk_spi_read_bytes(0, data, size);
#else
    ADDLOG_ERROR(LOG_FEATURE_DRV, "SPI_ReadBytes not supported");
    return -1;
#endif
}

int SPI_Transmit(const void *txData, uint32_t txSize, void *rxData,
        uint32_t rxSize) {
#if PLATFORM_BK7231N
	return bk_spi_transmit(0, txData, txSize, rxData, rxSize);
#else
    ADDLOG_ERROR(LOG_FEATURE_DRV, "SPI_Transmit not supported");
    return -1;
#endif
}
