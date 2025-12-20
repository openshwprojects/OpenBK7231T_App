#if PLATFORM_BK7231N || PLATFORM_BEKEN_NEW
#include "arm_arch.h"

#if PLATFORM_BEKEN_NEW
#include "sys_config.h"
#endif
#include "drv_model_pub.h"
#include "general_dma_pub.h"
#include "gpio_pub.h"
#include "icu_pub.h"
#include "spi_pub.h"

#include "../../beken378/driver/general_dma/general_dma.h"
#if PLATFORM_BEKEN_NEW
#if !PLATFORM_BK7231N && !PLATFORM_BK7238 && !PLATFORM_BK7252N
#undef CFG_SOC_NAME
#define CFG_SOC_NAME 5
#endif
#include "spi_bk7231n.h"
#else
#include "../../beken378/driver/spi/spi.h"
#endif

#include "../new_cfg.h"
#include "../new_common.h"
#include "../new_pins.h"

// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../hal/hal_pins.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "../mqtt/new_mqtt.h"

#include "drv_spidma.h"

static struct bk_spi_dev *spi_dev;

static void setSpiCtrlRegister(unsigned long bit, BOOLEAN val) {
	UINT32 value;

	value = REG_READ(SPI_CTRL);
	if (val == 0) {
		value &= ~bit;
	} else if (val == 1) {
		value |= bit;
	}
	REG_WRITE(SPI_CTRL, value);
}

static void setSpiConfigRegister(unsigned long bit, BOOLEAN val) {
	UINT32 value;

	value = REG_READ(SPI_CONFIG);
	if (val == 0) {
		value &= ~bit;
	} else if (val == 1) {
		value |= bit;
	}
	REG_WRITE(SPI_CONFIG, value);
}

static void set_txtrans_len(UINT32 val) {
	UINT32 value;

	value = REG_READ(SPI_CONFIG);

	value &= ~(0xFFF << SPI_TX_TRAHS_LEN_POSI);
	value |= ((val & 0xFFF) << SPI_TX_TRAHS_LEN_POSI);

	REG_WRITE(SPI_CONFIG, value);
}

void spidma_dma_tx_enable(UINT8 enable) {
	int param;
	GDMA_CFG_ST en_cfg;

	//enable tx
	param = enable;
	setSpiConfigRegister(SPI_TX_EN, param);
	//sddev_control(SPI_DEV_NAME, CMD_SPI_TX_EN, (void *)&param);
	en_cfg.channel = SPI_TX_DMA_CHANNEL;

	if (enable)
		en_cfg.param = 1;
	else
		en_cfg.param = 0;
	sddev_control(GDMA_DEV_NAME, CMD_GDMA_SET_DMA_ENABLE, &en_cfg);
}


static void spi_set_nssmd(UINT8 val)
{
	UINT32 value;

	value = REG_READ(SPI_CTRL);
	value &= ~CTRL_NSSMD_3;
	value |= (val << 17);
	REG_WRITE(SPI_CTRL, value);

}

static void spi_set_clock(UINT32 max_hz)
{
	int source_clk = 0;
	int spi_clk = 0;
	int div = 0;
	UINT32 param;
	//os_printf("\rmax_hz :%d\r\n", max_hz);

	if ((max_hz == 26000000) || (max_hz == 13000000) || (max_hz == 6500000)) {
		BK_SPI_PRT("config spi clk source 26MHz\n");

		spi_clk = max_hz;

#if CFG_XTAL_FREQUENCE

		source_clk = CFG_XTAL_FREQUENCE;
#else
		source_clk = SPI_PERI_CLK_26M;
#endif
		param = PCLK_POSI_SPI;
		sddev_control(ICU_DEV_NAME, CMD_CONF_PCLK_26M, &param);
	} else if (max_hz > 4333000) {
		BK_SPI_PRT("config spi clk source DCO\n");

		if (max_hz > 30000000) { // 120M/2 / (2 + 1) = 30M
			spi_clk = 30000000;
			BK_SPI_PRT("input clk > 30MHz, set input clk = 30MHz\n");
		} else
			spi_clk = max_hz;
		sddev_control(ICU_DEV_NAME, CMD_CLK_PWR_DOWN, &param);
		source_clk = SPI_PERI_CLK_DCO;
		param = PCLK_POSI_SPI;
		sddev_control(ICU_DEV_NAME, CMD_CONF_PCLK_DCO, &param);
		param = PWD_SPI_CLK_BIT;
		sddev_control(ICU_DEV_NAME, CMD_CLK_PWR_UP, &param);
	} else {
		BK_SPI_PRT("config spi clk source 26MHz\n");

		spi_clk = max_hz;
#if CFG_XTAL_FREQUENCE
		source_clk = CFG_XTAL_FREQUENCE;
#else
		source_clk = SPI_PERI_CLK_26M;
#endif

		param = PCLK_POSI_SPI;
		sddev_control(ICU_DEV_NAME, CMD_CONF_PCLK_26M, &param);
	}
	if ((max_hz == 26000000) || (max_hz == 13000000) || (max_hz == 6500000))
		div = source_clk / spi_clk - 1;
	else {
		// spi_clk = in_clk / (2 * (div + 1))
		div = ((source_clk >> 1) / spi_clk);

		if (div < 2)
			div = 2;
		else if (div >= 255)
			div = 255;
	}

	param = REG_READ(SPI_CTRL);
	param &= ~(SPI_CKR_MASK << SPI_CKR_POSI);
	param |= (div << SPI_CKR_POSI);
	REG_WRITE(SPI_CTRL, param);

	BK_SPI_PRT("\rdiv = %d \r\n", div);
	BK_SPI_PRT("\rspi_clk = %d \r\n", spi_clk);
	BK_SPI_PRT("\rsource_clk = %d \r\n", source_clk);
	BK_SPI_PRT("\rtarget frequency = %d, actual frequency = %d \r\n", max_hz, source_clk / 2 / div);
}

static void spidma_spi_configure(UINT32 rate, UINT32 mode) {
	UINT32 param;
	//struct spi_callback_des spi_dev_cb;

	param = 0;
	setSpiCtrlRegister(MSTEN, param);
	//sddev_control(SPI_DEV_NAME, CMD_SPI_INIT_MSTEN, (void *)&param);

	/* data bit width */
	param = 0;
	setSpiCtrlRegister(BIT_WDTH, param); //ToDo: check
	//sddev_control(SPI_DEV_NAME, CMD_SPI_SET_BITWIDTH, (void *)&param);

	/* baudrate */
	ADDLOG_EXTRADEBUG(LOG_FEATURE_CMD, "max_hz = %d \n", rate);
	//setSpiCtrlRegister(BIT_WDTH, param);
	spi_set_clock(rate);
	//sddev_control(SPI_DEV_NAME, CMD_SPI_SET_CKR, (void *)&rate);

	/* mode */
	if (mode & BK_SPI_CPOL)
		param = 1;
	else
		param = 0;
	setSpiCtrlRegister(CKPOL, param);
	//sddev_control(SPI_DEV_NAME, CMD_SPI_SET_CKPOL, (void *)&param);

	/* CPHA */
	if (mode & BK_SPI_CPHA)
		param = 1;
	else
		param = 0;
	setSpiCtrlRegister(CKPHA, param);
	//sddev_control(SPI_DEV_NAME, CMD_SPI_SET_CKPHA, (void *)&param);

	/* Master */
	param = 1;
	setSpiCtrlRegister(MSTEN, param);
	//sddev_control(SPI_DEV_NAME, CMD_SPI_SET_MSTEN, (void *)&param);
	param = 1;
	//setSpiCtrlRegister(MSTEN, param);
	spi_set_nssmd(param);
	//sddev_control(SPI_DEV_NAME, CMD_SPI_SET_NSSMD, (void *)&param);

	/* set call back func */
	//	spi_dev_cb.callback = bk_spi_rx_callback;
	//	spi_dev_cb.param = NULL;
	//	sddev_control(SPI_DEV_NAME, CMD_SPI_SET_RX_CALLBACK, (void *)&spi_dev_cb);
	//
	//	spi_dev_cb.callback = bk_spi_tx_needwrite_callback;
	//	spi_dev_cb.param = NULL;
	//	sddev_control(SPI_DEV_NAME, CMD_SPI_SET_TX_NEED_WRITE_CALLBACK, (void *)&spi_dev_cb);
	//
	//	spi_dev_cb.callback = bk_spi_tx_finish_callback;
	//	spi_dev_cb.param = NULL;
	//	sddev_control(SPI_DEV_NAME, CMD_SPI_SET_TX_FINISH_CALLBACK, (void *)&spi_dev_cb);

	/* enable spi */
	param = 1;
	setSpiCtrlRegister(SPIEN, param);
	//sddev_control(SPI_DEV_NAME, CMD_SPI_UNIT_ENABLE, (void *)&param);


	ADDLOG_EXTRADEBUG(LOG_FEATURE_CMD, "spi_master:[CTRL]:0x%08x \n", REG_READ(SPI_CTRL));
}

static void spidma_spi_unconfigure(void) {
	sddev_control(SPI_DEV_NAME, CMD_SPI_DEINIT_MSTEN, NULL);
}

int spidma_spi_master_deinit(void) {
	if (spi_dev == NULL)
		return 0;

	if (spi_dev->mutex)
		rtos_lock_mutex(&spi_dev->mutex);

	if (spi_dev->tx_sem)
		rtos_deinit_semaphore(&spi_dev->tx_sem);
	if (spi_dev->rx_sem)
		rtos_deinit_semaphore(&spi_dev->rx_sem);
	if (spi_dev->dma_tx_sem)
		rtos_deinit_semaphore(&spi_dev->dma_tx_sem);
	if (spi_dev->dma_rx_sem)
		rtos_deinit_semaphore(&spi_dev->dma_rx_sem);

	if (spi_dev->mutex) {
		rtos_unlock_mutex(&spi_dev->mutex);
		rtos_deinit_mutex(&spi_dev->mutex);
	}

	if (spi_dev) {
		os_free(spi_dev);
		spi_dev = NULL;
	}

	spidma_spi_unconfigure();

	return 0;
}

void spidma_spi_dma_tx_finish_callback(UINT32 param) {

	spi_dev->flag &= ~(TX_FINISH_FLAG);
	rtos_set_semaphore(&spi_dev->dma_tx_sem);

	//ToDo: USer
	ADDLOG_EXTRADEBUG(LOG_FEATURE_CMD, "spi dma tx finish callback\r\n");
	spidma_dma_tx_enable(0);
}


int spidma_dma_master_tx_init(struct spi_message *spi_msg) {
	GDMACFG_TPYES_ST init_cfg;
	GDMA_CFG_ST en_cfg;

	ADDLOG_EXTRADEBUG(LOG_FEATURE_CMD, "spi dma tx init\r\n");
	os_memset(&init_cfg, 0, sizeof(GDMACFG_TPYES_ST));
	os_memset(&en_cfg, 0, sizeof(GDMA_CFG_ST));

	init_cfg.dstdat_width = 8;
	init_cfg.srcdat_width = 32;
	init_cfg.dstptr_incr = 0;
	init_cfg.srcptr_incr = 1;

	init_cfg.src_start_addr = spi_msg->send_buf;
	init_cfg.dst_start_addr = (void *)SPI_DAT;

	init_cfg.channel = SPI_TX_DMA_CHANNEL;
	init_cfg.prio = 10;
	init_cfg.u.type4.src_loop_start_addr = spi_msg->send_buf;
	init_cfg.u.type4.src_loop_end_addr = spi_msg->send_buf + spi_msg->send_len;

	init_cfg.half_fin_handler = NULL;
	init_cfg.fin_handler = spidma_spi_dma_tx_finish_callback;
	//spi_ctrl(CMD_SPI_SET_TX_FINISH_INT_CALLBACK, (void *)spidma_spi_dma_tx_finish_callback);

	init_cfg.src_module = GDMA_X_SRC_DTCM_RD_REQ;
	init_cfg.dst_module = GDMA_X_DST_GSPI_TX_REQ;

	sddev_control(GDMA_DEV_NAME, CMD_GDMA_CFG_TYPE4, (void *)&init_cfg);

	en_cfg.channel = SPI_TX_DMA_CHANNEL;
	en_cfg.param = spi_msg->send_len; // dma translen
	sddev_control(GDMA_DEV_NAME, CMD_GDMA_SET_TRANS_LENGTH, (void *)&en_cfg);

	ADDLOG_EXTRADEBUG(LOG_FEATURE_CMD, "spi dma tx config: length:%d, first:%d\r\n", spi_msg->send_len, spi_msg->send_buf[0]);

	en_cfg.channel = SPI_TX_DMA_CHANNEL;
	en_cfg.param = 0; // 0:not repeat 1:repeat
	sddev_control(GDMA_DEV_NAME, CMD_GDMA_CFG_WORK_MODE, (void *)&en_cfg);

	en_cfg.channel = SPI_TX_DMA_CHANNEL;
	en_cfg.param = 0; // src no loop
	sddev_control(GDMA_DEV_NAME, CMD_GDMA_CFG_SRCADDR_LOOP, &en_cfg);

	return 0;
}

void spidma_spi_master_dma_config(UINT32 mode, UINT32 rate) {
	UINT32 param;
	ADDLOG_EXTRADEBUG(LOG_FEATURE_CMD, "spi master dma init: mode:%d, rate:%d\r\n", mode, rate);
	spidma_spi_configure(rate, mode);

	//disable tx/rx int disable
	param = 0;
	setSpiCtrlRegister(TXINT_EN, param);
	//sddev_control(SPI_DEV_NAME, CMD_SPI_TXINT_EN, (void *)&param);

	param = 0;
	setSpiCtrlRegister(RXINT_EN, param);
	//sddev_control(SPI_DEV_NAME, CMD_SPI_RXINT_EN, (void *)&param);

	//disable rx/tx finish enable bit
	param = 1;
	setSpiConfigRegister(SPI_TX_FINISH_EN, param);
	//sddev_control(SPI_DEV_NAME, CMD_SPI_TXFINISH_EN, (void *)&param);

	param = 1;
	setSpiConfigRegister(SPI_RX_FINISH_EN, param);
	//sddev_control(SPI_DEV_NAME, CMD_SPI_RXFINISH_EN, (void *)&param);

	//disable rx/tx over
	param = 0;
	setSpiCtrlRegister(RXOVR_EN, param);
	//sddev_control(SPI_DEV_NAME, CMD_SPI_RXOVR_EN, (void *)&param);

	param = 0;
	setSpiCtrlRegister(TXOVR_EN, param);
	//sddev_control(SPI_DEV_NAME, CMD_SPI_TXOVR_EN, (void *)&param);

	param = 1;
	spi_set_nssmd(param);
	//sddev_control(SPI_DEV_NAME, CMD_SPI_SET_NSSMD, (void *)&param);
	//sddev_control(SPI_DEV_NAME, CMD_SPI_SET_3_LINE, NULL);
	uint32_t val;

	val = GFUNC_MODE_SPI_USE_GPIO_14;
	sddev_control(GPIO_DEV_NAME, CMD_GPIO_ENABLE_SECOND, &val);

	val = GFUNC_MODE_SPI_USE_GPIO_16_17;
 	sddev_control(GPIO_DEV_NAME, CMD_GPIO_ENABLE_SECOND, &val);


	param = 0;
	setSpiCtrlRegister(BIT_WDTH, param);
	//sddev_control(SPI_DEV_NAME, CMD_SPI_SET_BITWIDTH, (void *)&param);

	//disable CSN intterrupt
	param = 0;
	setSpiCtrlRegister(SPI_S_CS_UP_INT_EN, param);
	//sddev_control(SPI_DEV_NAME, CMD_SPI_CS_EN, (void *)&param);

	//clk test
	//param = 5000000;
	//sddev_control(SPI_DEV_NAME, CMD_SPI_SET_CKR, (void *)&param);

	ADDLOG_EXTRADEBUG(LOG_FEATURE_CMD, "spi_master [CTRL]:0x%08x \n", REG_READ(SPI_CTRL));
	ADDLOG_EXTRADEBUG(LOG_FEATURE_CMD, "spi_master [CONFIG]:0x%08x \n", REG_READ(SPI_CONFIG));
}

int spidma_spi_master_dma_tx_init(UINT32 mode, UINT32 rate, struct spi_message *spi_msg) {
	OSStatus result = 0;

	if (spi_dev)
		spidma_spi_master_deinit();

	spi_dev = os_malloc(sizeof(struct bk_spi_dev));
	if (!spi_dev) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "[spi]:malloc memory for spi_dev failed\n");
		result = -1;
		goto _exit;
	}
	os_memset(spi_dev, 0, sizeof(struct bk_spi_dev));

	result = rtos_init_semaphore(&spi_dev->dma_tx_sem, 1);
	if (result != kNoErr) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "[spi]: spi tx semp init failed\n");
		goto _exit;
	}

	spidma_spi_master_dma_config(mode, rate);

	spidma_dma_master_tx_init(spi_msg);
	spidma_dma_tx_enable(0);
	set_txtrans_len((spi_msg->send_len));
	//sddev_control(SPI_DEV_NAME, CMD_SPI_TXTRANS_EN, (void *)&spi_msg->send_len);

	spi_dev->init_dma_tx = 1;
	spi_dev->tx_ptr = spi_msg->send_buf;
	spi_dev->tx_len = spi_msg->send_len;

	return 0;

_exit:

	spidma_spi_master_deinit();

	return 1;
}


int spidma_spi_master_dma_send(struct spi_message *spi_msg) {
	int ret = 0;

	GLOBAL_INT_DECLARATION();
	ASSERT(spi_msg != NULL);
	if (spi_dev->init_dma_tx == 0 || spi_dev == NULL) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "spi_dma_send_ no init!\n");
		return -1;
	}
	if (spi_dev->flag & TX_FINISH_FLAG) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "spi_dma_send_ TX_FINISH_FLAG!\n");
		return -2;
	}

	if (spi_dev->tx_ptr != spi_msg->send_buf) {
		ADDLOG_EXTRADEBUG(LOG_FEATURE_CMD, "spi_dma_send_ set pointer!\n");

		GDMA_CFG_ST en_cfg;
		en_cfg.channel = SPI_TX_DMA_CHANNEL;
		en_cfg.param = (UINT32)spi_msg->send_buf; // dma dst addr
		sddev_control(GDMA_DEV_NAME, CMD_GDMA_SET_SRC_START_ADDR, (void *)&en_cfg);
		spi_dev->tx_ptr = spi_msg->send_buf;
	}
	if (spi_dev->tx_len != spi_msg->send_len) {
		ADDLOG_EXTRADEBUG(LOG_FEATURE_CMD, "spi_dma_send_ set length!\n");

		GDMA_CFG_ST en_cfg;
		//spi_ctrl(CMD_SPI_TXTRANS_EN, (void *)&spi_msg->send_len);
		set_txtrans_len((spi_msg->send_len));
		en_cfg.channel = SPI_TX_DMA_CHANNEL;
		en_cfg.param = spi_msg->send_len; // dma translen
		sddev_control(GDMA_DEV_NAME, CMD_GDMA_SET_TRANS_LENGTH, (void *)&en_cfg);
		spi_dev->tx_len = spi_msg->send_len;
	}

	GLOBAL_INT_DISABLE();
	spi_dev->flag |= TX_FINISH_FLAG;
	GLOBAL_INT_RESTORE();
	ADDLOG_EXTRADEBUG(LOG_FEATURE_CMD, "before enable tx 0x%08x\r\n", REG_READ(SPI_CONFIG));

	spidma_dma_tx_enable(1);
	ADDLOG_EXTRADEBUG(LOG_FEATURE_CMD, "enable tx 0x%08x\r\n", REG_READ(SPI_CONFIG));
	/* wait tx finish */
	//if (user_dma_tx_finish_callback == NULL) {
	//因为写DMA是主动操作，所以DMA传输完成后，SPI不一定发送完成了。

	rtos_get_semaphore(&spi_dev->dma_tx_sem, BEKEN_NEVER_TIMEOUT);
	ret = spi_dev->flag;
	//ADDLOG_ERROR(LOG_FEATURE_CMD, "rtos_semaphore ret 0x%08x \n", ret);
	//} else
	//ret = 0;

	if (spi_msg->send_buf != NULL)
		return ret;
	else {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "spi_dma tx error send_buff\r\n", spi_msg->send_buf);
		return -3;
	}
}
void spidma_master_dma_tx_disable(void) {
	spidma_dma_tx_enable(0);
	spi_dev->flag &= (~TX_FINISH_FLAG);
}

void SPIDMA_Init(struct spi_message *spi_msg) {
	uint32_t val;

	val = GFUNC_MODE_SPI_USE_GPIO_14;
	sddev_control(GPIO_DEV_NAME, CMD_GPIO_ENABLE_SECOND, &val);

	val = GFUNC_MODE_SPI_USE_GPIO_16_17;
	sddev_control(GPIO_DEV_NAME, CMD_GPIO_ENABLE_SECOND, &val);

	UINT32 param;
	param = PCLK_POSI_SPI;
	sddev_control(ICU_DEV_NAME, CMD_CONF_PCLK_26M, &param);

	param = PWD_SPI_CLK_BIT;
	sddev_control(ICU_DEV_NAME, CMD_CLK_PWR_UP, &param);

	spidma_spi_master_dma_tx_init(0, 3000000, spi_msg);
}

void SPIDMA_StartTX(struct spi_message *spi_msg) {
	spidma_spi_master_dma_send(spi_msg);
}

void SPIDMA_StopTX() {
	spidma_master_dma_tx_disable();
}

void SPIDMA_Deinit(void)
{

}

#elif PLATFORM_ESPIDF

#include "../new_cfg.h"
#include "../new_common.h"
#include "../new_pins.h"
#include "../hal/espidf/hal_pinmap_espidf.h"

#include "drv_spidma.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "soc/spi_reg.h"

#if SOC_SPI_PERIPH_NUM > 2
spi_host_device_t obk_spi_host = SPI3_HOST;
#else
spi_host_device_t obk_spi_host = SPI2_HOST;
#endif

spi_device_handle_t obk_spidma;
extern int spidma_led_pin;

void SPIDMA_Init(struct spi_message* msg)
{
	spi_bus_config_t buscfg = 
	{
		.miso_io_num = -1,
		.mosi_io_num = spidma_led_pin > 0 ? (int)g_pins[spidma_led_pin].pin : -1,
		.sclk_io_num = -1,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.max_transfer_sz = msg->send_len,
	};

	spi_device_interface_config_t devcfg = 
	{
		.clock_speed_hz = 3000000,
		.mode = 0,
		.spics_io_num = -1,
		.queue_size = 1,
		.command_bits = 0,
		.address_bits = 0,
	};

	spi_bus_initialize(obk_spi_host, &buscfg, SPI_DMA_CH_AUTO);
	spi_bus_add_device(obk_spi_host, &devcfg, &obk_spidma);
}

void SPIDMA_StartTX(struct spi_message* msg)
{
	spi_transaction_t transaction = { 0 };
	transaction.length = msg->send_len * 8; // bits, not bytes
	transaction.tx_buffer = msg->send_buf;
	spi_device_transmit(obk_spidma, &transaction);
}

void SPIDMA_StopTX(void)
{

}

void SPIDMA_Deinit(void)
{
	spi_bus_remove_device(obk_spidma);
	spi_bus_free(obk_spi_host);
}

#elif PLATFORM_LN882H

// this uses SPI + DMA
// for native implementation, see https://github.com/openshwprojects/OpenBK7231T_App/pull/1414

#include "../new_cfg.h"
#include "../new_common.h"
#include "../new_pins.h"

#include "drv_spidma.h"
#include "../hal/ln882h/hal_pinmap_ln882h.h"
#include "hal/hal_dma.h"
#include "hal/hal_spi.h"

extern int spidma_led_pin;
static int current_pin = 6;

void SPIDMA_Init(struct spi_message* msg)
{
	current_pin = spidma_led_pin > 0 ? spidma_led_pin : 6;
	lnPinMapping_t* pin = g_pins + current_pin;

	hal_gpio_pin_afio_select(pin->base, pin->pin, SPI0_MOSI);
	hal_gpio_pin_afio_en(pin->base, pin->pin, HAL_ENABLE);
	
	spi_init_type_def spi_init =
	{
		.spi_baud_rate_prescaler = SPI_BAUDRATEPRESCALER_16,
		.spi_mode = SPI_MODE_MASTER,
		.spi_data_size = SPI_DATASIZE_8B,
		.spi_first_bit = SPI_FIRST_BIT_MSB,
		.spi_cpol = SPI_CPOL_LOW,
		.spi_cpha = SPI_CPHA_1EDGE,
	};
	
	hal_spi_init(SPI0_BASE, &spi_init);
	hal_spi_en(SPI0_BASE, HAL_ENABLE);
	hal_spi_ssoe_en(SPI0_BASE, HAL_DISABLE);
	
	dma_init_t_def dma_init =
	{
		.dma_mem_addr = (uint32_t)msg->send_buf,
		.dma_data_num = msg->send_len,
		.dma_dir = DMA_READ_FORM_MEM,
		.dma_mem_inc_en = DMA_MEM_INC_EN,
		.dma_p_addr = SPI0_DATA_REG,
		.dma_p_size = DMA_P_SIZE_8_BIT,
		.dma_mem_size = DMA_MEM_SIZE_8_BIT,
	};
	
	hal_dma_init(DMA_CH_4, &dma_init);
	hal_dma_en(DMA_CH_4, HAL_DISABLE);
}

void SPIDMA_StartTX(struct spi_message* msg)
{
	hal_dma_set_mem_addr(DMA_CH_4, (uint32_t)msg->send_buf);
	hal_dma_set_data_num(DMA_CH_4, msg->send_len);

	hal_spi_dma_en(SPI0_BASE, SPI_DMA_TX_EN, HAL_ENABLE);
	hal_dma_en(DMA_CH_4, HAL_ENABLE);
	
	while(hal_dma_get_data_num(DMA_CH_4) != 0);
	
	hal_dma_en(DMA_CH_4, HAL_DISABLE);
	hal_spi_dma_en(SPI0_BASE, SPI_DMA_TX_EN, HAL_DISABLE);
}

void SPIDMA_StopTX(void)
{

}

void SPIDMA_Deinit(void)
{
	hal_spi_en(SPI0_BASE, HAL_DISABLE);
	hal_spi_deinit(SPI0_BASE);
	lnPinMapping_t* pin = g_pins + current_pin;
	hal_gpio_pin_afio_en(pin->base, pin->pin, HAL_DISABLE);
}

#elif PLATFORM_REALTEK

#include "../hal/realtek/hal_pinmap_realtek.h"

#include "drv_spidma.h"
#include "spi_api.h"
#include "spi_ex_api.h"

static spi_t spi_master;
extern int spidma_led_pin;
rtlPinMapping_t* rtl_mosi;
bool is_init = false;

void SPIDMA_Init(struct spi_message* msg)
{
	is_init = false;
	PinName mosi = NC, miso = NC, sclk = NC, ssel = NC;
	rtl_mosi = g_pins + spidma_led_pin;
#if PLATFORM_RTL8710A
#define SPI_FREQ 4000000
	switch(rtl_mosi->pin)
	{
		case PE_2:
			sys_jtag_off();
			mosi = PE_2;
			miso = PE_3;
			break;
		case PC_2:
			mosi = PC_2;
			miso = PC_3;
			break;
		default: return;
	}
	pin_mode(mosi, PullDown);
#elif PLATFORM_RTL8710B
#define SPI_FREQ 3777777
	switch(rtl_mosi->pin)
	{
		case PA_4:
			spi_master.spi_idx = MBED_SPI0;
			mosi = PA_4;
			miso = PA_3;
			sclk = PA_1;
			ssel = PA_2;
			break;
		case PA_23:
			spi_master.spi_idx = MBED_SPI1;
			mosi = PA_23;
			miso = PA_22;
			sclk = PA_18;
			ssel = PA_19;
			break;
		case PB_3:
			spi_master.spi_idx = MBED_SPI1;
			mosi = PB_3;
			miso = PB_2;
			sclk = PB_1;
			ssel = PB_0;
			break;
		default: return;
	}
	pin_mode(mosi, PullDown);
#elif PLATFORM_RTL87X0C
#define SPI_FREQ 3000000
	// those pins aren't wired out on BW15, and, except for A7, on WBR3
	// they must be set, otherwise spi init will fail
	miso = PA_10;
	ssel = PA_7;
	sclk = PA_8;
	switch(rtl_mosi->pin)
	{
		case PA_4:
			mosi = PA_4;
			break;
		case PA_9:
			mosi = PA_9;
			break;
		case PA_19:
			mosi = PA_19;
			break;
		default: return;
	}
#elif PLATFORM_RTL8720D
#define SPI_FREQ 3000000
	switch(rtl_mosi->pin)
	{
		case PB_18:
			spi_master.spi_idx = MBED_SPI0;
			mosi = PB_18;
			break;
		case PA_16:
			spi_master.spi_idx = MBED_SPI0;
			mosi = PA_16;
			break;
		case PA_12:
			spi_master.spi_idx = MBED_SPI1;
			mosi = PA_12;
			break;
		case PB_4:
			spi_master.spi_idx = MBED_SPI1;
			mosi = PB_4;
			break;
		default: return;
	}
#elif PLATFORM_REALTEK_NEW
#define SPI_FREQ 3000000
	spi_master.spi_idx = MBED_SPI1;
	mosi = rtl_mosi->pin;
#endif
	spi_init(&spi_master, mosi, miso, sclk, ssel);
	spi_format(&spi_master, 8, 0, 0);
	spi_frequency(&spi_master, SPI_FREQ);
	is_init = true;
}
void SPIDMA_StartTX(struct spi_message* msg)
{
	if(is_init) spi_master_write_stream_dma(&spi_master, (char*)msg->send_buf, msg->send_len);
	//if(is_init) spi_master_write_stream(&spi_master, (char*)msg->send_buf, msg->send_len);
}
void SPIDMA_StopTX(void)
{

}
void SPIDMA_Deinit(void)
{
	if(is_init) spi_free(&spi_master);
}

#elif PLATFORM_BL602

#include "drv_spidma.h"
#include "hosal_spi.h"
#include "hosal_dma.h"
#include "bl602_dma.h"
#include "bl602_gpio.h"
#include "bl602_glb.h"
#include "bl602_spi.h"
#include "bl_irq.h"
#include "bl_dma.h"

extern int spidma_led_pin;
static hosal_dma_chan_t spidma_ch;
static DMA_LLI_Ctrl_Type spi_dma_lli[2];
bool is_init = false;

void SPIDMA_Init(struct spi_message* msg)
{
	is_init = false;
	switch(spidma_led_pin)
	{
		case 0:
		case 4:
		case 8:
		case 12:
		case 16:
		case 20:
			GLB_Swap_SPI_0_MOSI_With_MISO(DISABLE);
			break;
		case 1:
		case 5:
		case 9:
		case 13:
		case 17:
		case 21:
			GLB_Swap_SPI_0_MOSI_With_MISO(ENABLE);
			break;
		default: return;
	}
	GLB_GPIO_Func_Init(GPIO_FUN_SPI, (GLB_GPIO_Type*)&spidma_led_pin, 1);
	GLB_Set_SPI_0_ACT_MOD_Sel(GLB_SPI_PAD_ACT_AS_MASTER);

	SPI_CFG_Type spiCfg = 
	{
		DISABLE,
		ENABLE,
		SPI_BYTE_INVERSE_BYTE0_FIRST,
		SPI_BIT_INVERSE_MSB_FIRST,
		SPI_CLK_PHASE_INVERSE_0,
		SPI_CLK_POLARITY_LOW,
		SPI_FRAME_SIZE_8 
	};

	SPI_FifoCfg_Type fifoCfg =
	{
		1,
		0,
		ENABLE,
		DISABLE
	};

	spidma_ch = hosal_dma_chan_request(0);

	SPI_Disable(0, SPI_WORK_MODE_MASTER);
	SPI_Init(0, &spiCfg);
	SPI_FifoConfig(0, &fifoCfg);
	SPI_SetClock(0, 3000000);
	SPI_Enable(0, SPI_WORK_MODE_MASTER);

	DMA_LLI_Cfg_Type llicfg =
	{
		DMA_TRNS_M2P,
		DMA_REQ_NONE,
		DMA_REQ_SPI_TX,
	};

	DMA_LLI_Init(spidma_ch, &llicfg);

	struct DMA_Control_Reg dmactrl =
	{
		.TransferSize = msg->send_len,
		.SBSize = DMA_BURST_SIZE_1,
		.DBSize = DMA_BURST_SIZE_1,
		.SWidth = DMA_TRNS_WIDTH_8BITS,
		.DWidth = DMA_TRNS_WIDTH_8BITS,
		.SLargerD = 0,
		.SI = DMA_MINC_ENABLE,
		.DI = DMA_MINC_DISABLE,
		.Prot = 0,
		.I = 0,
	};

	spi_dma_lli[0].srcDmaAddr = (uint32_t)(msg->send_buf);
	spi_dma_lli[0].destDmaAddr = (uint32_t)(SPI_BASE + SPI_FIFO_WDATA_OFFSET);
	spi_dma_lli[0].dmaCtrl = dmactrl;
	spi_dma_lli[0].nextLLI = 0;
	is_init = true;
}

void SPIDMA_StartTX(struct spi_message* msg)
{
	if(is_init)
	{
		DMA_LLI_Update(spidma_ch, (uint32_t)spi_dma_lli);
		hosal_dma_chan_start(spidma_ch);
	}
}

void SPIDMA_StopTX(void)
{

}

void SPIDMA_Deinit(void)
{
	if(is_init) hosal_dma_chan_release(spidma_ch);
}

#else

#include "drv_spidma.h"

void SPIDMA_Init(struct spi_message* msg)
{

}
void SPIDMA_StartTX(struct spi_message* msg)
{

}
void SPIDMA_StopTX(void)
{

}
void SPIDMA_Deinit(void)
{

}

#endif
