#if PLATFORM_BK7231N
#include "../../beken378/func/user_driver/armino/spi/spi.h"
#else
// spi_config_t and its member types are copied from BK7321N SPI implementation.
#include "drv_spi.h"
#if PLATFORM_BK7231T
#include "../../../../platforms/bk7231t/bk7231t_os/beken378/common/typedef.h"
#include "../../../../platforms/bk7231t/bk7231t_os/beken378/driver/entry/arch.h"
#include "../../../../platforms/bk7231t/bk7231t_os/beken378/driver/include/drv_model_pub.h"
#include "../../../../platforms/bk7231t/bk7231t_os/beken378/driver/include/gpio_pub.h"
#include "../../../../platforms/bk7231t/bk7231t_os/beken378/driver/include/icu_pub.h"
#include "../../../../platforms/bk7231t/bk7231t_os/beken378/driver/include/spi_pub.h"
#include "../../../../platforms/bk7231t/bk7231t_os/beken378/os/FreeRTOSv9.0.0/FreeRTOS/Source/portable/Keil/ARM968es/portmacro.h"
#endif // PLATFORM_BK7231T
#endif // PLATFORM_BK7231N

#include "../logging/logging.h"

int SPI_DriverInit(void) {
#if PLATFORM_BK7231N
    return bk_spi_driver_init();
#elif PLATFORM_BK7231T
	// Is called in dd.c
	// spi_init();
	return 0;
#else
    ADDLOG_ERROR(LOG_FEATURE_DRV, "SPI_DriverInit not supported");
    return -1;
#endif
}

int SPI_DriverDeinit(void) {
#if PLATFORM_BK7231N
    return bk_spi_driver_deinit();
#elif PLATFORM_BK7231T
	// Is called in dd.c
	// spi_exit();
	return 0;
#else
    ADDLOG_ERROR(LOG_FEATURE_DRV, "SPI_DriverDeinit not supported");
    return -1;
#endif
}

int SPI_Init(const spi_config_t *config) {
#if PLATFORM_BK7231N
	return bk_spi_init(0, config);
#elif PLATFORM_BK7231T
	int err = 0;

	uint32_t param = PWD_SPI_CLK_BIT;
	err |= sddev_control(ICU_DEV_NAME, CMD_CLK_PWR_UP, &param);

	param = PCLK_POSI_SPI;
	err |= sddev_control(ICU_DEV_NAME, CMD_CONF_PCLK_26M, &param);

	param = (IRQ_SPI_BIT);
	err |= sddev_control(ICU_DEV_NAME, CMD_ICU_INT_ENABLE, &param);

	param = GFUNC_MODE_SPI;
	err |= sddev_control(GPIO_DEV_NAME, CMD_GPIO_ENABLE_SECOND, &param);

	// First CMD_SPI_INIT_MSTEN, because it overwrites the following settings
	spi_msten mst = (config->role == SPI_ROLE_MASTER ? master : slave);
	err |= sddev_control(SPI_DEV_NAME, CMD_SPI_INIT_MSTEN, &mst);

	uint8_t clk_div = 26000000 / config->baud_rate;
	if (clk_div == 0)
		clk_div = 1;
	err |= sddev_control(SPI_DEV_NAME, CMD_SPI_SET_CKR, &clk_div);

	uint8_t bitwidth = (config->bit_width == SPI_BIT_WIDTH_8BITS ? 0 : 1);
	err |= sddev_control(SPI_DEV_NAME, CMD_SPI_SET_BITWIDTH, &bitwidth);

	if (config->bit_order == SPI_LSB_FIRST) {
		// Could be implemented with manual bit reverse
		ADDLOG_ERROR(LOG_FEATURE_DRV, "SPI_LSB_FIRST not supported");
		return -1;
	}

	uint8_t ckpol = (config->polarity == SPI_POLARITY_LOW ? 0 : 1);
	err |= sddev_control(SPI_DEV_NAME, CMD_SPI_SET_CKPOL, &ckpol);

	uint8_t ckpha = (config->phase == SPI_PHASE_1ST_EDGE ? 0 : 1);
	err |= sddev_control(SPI_DEV_NAME, CMD_SPI_SET_CKPHA, &ckpha);

	// config->wire_mode

	spi_trans.tx_remain_data_cnt = 0;
	spi_trans.rx_remain_data_cnt = 0;
	spi_trans.p_tx_buf = 0;
	spi_trans.p_rx_buf = 0;
	spi_trans.trans_done = 1;

	return err;
#else
    ADDLOG_ERROR(LOG_FEATURE_DRV, "SPI_Init not supported");
    return -1;
#endif
}

int SPI_Deinit(void) {
#if PLATFORM_BK7231N
	return bk_spi_deinit(0);
#elif PLATFORM_BK7231T
	int err = 0;

	uint8_t enable = 0;
	err |= sddev_control(SPI_DEV_NAME, CMD_SPI_UNIT_ENABLE, &enable);

	uint32_t param = (IRQ_SPI_BIT);
	err |= sddev_control(ICU_DEV_NAME, CMD_ICU_INT_DISABLE, &param);

	param = PWD_SPI_CLK_BIT;
	err |= sddev_control(ICU_DEV_NAME, CMD_CLK_PWR_DOWN, &param);

	return err;
#else
    ADDLOG_ERROR(LOG_FEATURE_DRV, "SPI_Deinit not supported");
    return -1;
#endif
}

#if PLATFORM_BK7231T
static inline int Spi_wait_for_ready() {
	uint8_t busy = 0;
	int err = 0;

	do {
		err = sddev_control(SPI_DEV_NAME, CMD_SPI_GET_BUSY, &busy);
	} while (busy || !err);

	return err;
}
#endif

int SPI_WriteBytes(const void *data, uint32_t size) {
#if PLATFORM_BK7231N
    return bk_spi_write_bytes(0, data, size);
#elif PLATFORM_BK7231T
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
		
	//int err = Spi_wait_for_ready();

	spi_trans.tx_remain_data_cnt = size;
	spi_trans.rx_remain_data_cnt = 0;
	spi_trans.p_tx_buf = (uint8_t *)data;
	spi_trans.p_rx_buf = 0;
	spi_trans.trans_done = 0;

	uint32_t not_used = 0;
	int err = sddev_control(SPI_DEV_NAME, CMD_SPI_TXFIFO_CLR, &not_used);

	not_used = 0;
	err |= sddev_control(SPI_DEV_NAME, CMD_SPI_START_TRANS, &not_used);

    GLOBAL_INT_RESTORE();

	err |= Spi_wait_for_ready();
	return err;
#else
    ADDLOG_ERROR(LOG_FEATURE_DRV, "SPI_WriteBytes not supported");
    return -1;
#endif
}

int SPI_ReadBytes(void *data, uint32_t size) {
#if PLATFORM_BK7231N
	return bk_spi_read_bytes(0, data, size);
#elif PLATFORM_BK7231T
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

    //int err = Spi_wait_for_ready();

    spi_trans.tx_remain_data_cnt = 0;
	spi_trans.rx_remain_data_cnt = size;
	spi_trans.p_tx_buf = 0;
	spi_trans.p_rx_buf = (uint8_t *)data;
	spi_trans.trans_done = 0;

	uint32_t not_used = 0;
	int err = sddev_control(SPI_DEV_NAME, CMD_SPI_RXFIFO_CLR, &not_used);

	not_used = 0;
	err |= sddev_control(SPI_DEV_NAME, CMD_SPI_START_TRANS, &not_used);

    GLOBAL_INT_RESTORE();

	err |= Spi_wait_for_ready();
	return err;
#else
    ADDLOG_ERROR(LOG_FEATURE_DRV, "SPI_ReadBytes not supported");
    return -1;
#endif
}

int SPI_Transmit(const void *txData, uint32_t txSize, void *rxData,
        uint32_t rxSize) {
#if PLATFORM_BK7231N
	return bk_spi_transmit(0, txData, txSize, rxData, rxSize);
#elif PLATFORM_BK7231T
	int err = 0;

    if (txSize && txData)
		err |= SPI_WriteBytes(txData, txSize);

    if (rxSize && rxData)
		err |= SPI_ReadBytes(rxData, rxSize);

	return err;
#else
    ADDLOG_ERROR(LOG_FEATURE_DRV, "SPI_Transmit not supported");
    return -1;
#endif
}
