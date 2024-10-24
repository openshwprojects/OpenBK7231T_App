/**
 * @file wm_gpio_afsel.h
 *
 * @brief GPIO Driver Module
 *
 * @author dave
 *
 * Copyright (c) 2014 Winner Microelectronics Co., Ltd.
 */
#ifndef WM_GPIO_AFSEL_H
#define WM_GPIO_AFSEL_H

#include "wm_gpio.h"
#include "wm_regs.h"
#include "wm_irq.h"
#include "wm_osal.h"
#include "tls_common.h"
/**
 * @addtogroup Driver_APIs
 * @{
 */

/**
 * @defgroup IOMUX_Driver_APIs IOMUX Driver APIs
 * @brief IO Multiplex driver APIs
 */

/**
 * @addtogroup IOMUX_Driver_APIs
 * @{
 */


/**
 * @brief  config the pins used for highspeed spi
 * @param  numsel: config highspeed spi pins multiplex relation,valid para 0,1
 *			 0: hspi0 		  1: hspi1 only for 56pin
 *			    hspi_ck  PB06      hspi_ck  PB12
 *			    hspi_int PB07      hspi_int PB13
 *			    hspi_cs  PB09      hspi_cs  PB14
 *			    hspi_di  PB10      hspi_di  PB15
 *			    hspi_do	PB11      hspi_do	 PB16
 * @return None
 */
void wm_hspi_gpio_config(uint8_t numsel);

/**
 * @brief  config the pins used for spi ck
 * @param  io_name: config spi ck pins name
 *			WM_IO_PB_01
 *			WM_IO_PB_02
 *			WM_IO_PB_15 only for 56pin
 *			WM_IO_PB_24 only for 56pin
 *				
 * @return None
 */
void wm_spi_ck_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for spi cs
 * @param  io_name: config spi cs pins name
 *			WM_IO_PA_00
 *			WM_IO_PB_04
 *			WM_IO_PB_14 only for 56pin
 *			WM_IO_PB_23 only for 56pin 
 *				
 * @return None
 */
void wm_spi_cs_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for spi di
 * @param  io_name: config spi di pins name
 *			WM_IO_PB_00
 *			WM_IO_PB_03
 *			WM_IO_PB_16 only for 56pin
 *			WM_IO_PB_25 only for 56pin  
 *				
 * @return None
 */
void wm_spi_di_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for spi do
 * @param  io_name: config spi do pins name
 *			WM_IO_PA_07
 *			WM_IO_PB_05
 *			WM_IO_PB_17 only for 56pin
 *			WM_IO_PB_26 only for 56pin  
 *				
 * @return None
 */
void wm_spi_do_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for sdio host ck dat0 dat1 dat2 dat3
 * @param  numsel: config sdio ck cmd dat0 dat1 dat2 dat3 pins multiplex relation,valid para 0,1
 *			0:                 1: only for 56pin
 *			  sdio_ck   PB06     sdio_ck   PA09
 *			  sdio_cmd  PB07     sdio_cmd  PA10
 *			  sdio_dat0 PB08     sdio_dat0 PA11
 *			  sdio_dat1 PB09     sdio_dat1 PA12
 *			  sdio_dat2 PB10     sdio_dat2 PA13
 *			  sdio_dat3 PB11     sdio_dat3 PA14
 *				
 * @return None
 */
void wm_sdio_host_config(uint8_t numsel);

/**
 * @brief  config the pins used for sdio slave ck dat0 dat1 dat2 dat3
 * @param  numsel: config sdio ck cmd dat0 dat1 dat2 dat3 pins multiplex relation,valid para 0
 *			0: 
 *			  sdio_ck   PB06
 *            sdio_cmd  PB07
 *			  sdio_dat0 PB08
 *			  sdio_dat1 PB09
 *			  sdio_dat2 PB10
 *			  sdio_dat3 PB11
 *				
 * @return None
 */
void wm_sdio_slave_config(uint8_t numsel);

/**
 * @brief  config the pins used for psram ck cs dat0 dat1 dat2 dat3
 * @param  numsel: config psram ck cs dat0 dat1 dat2 dat3 pins multiplex relation,valid para 0,1
 *			0:                 1: only for 56pin
 *			  psram_ck   PB00    psram_ck   PA15
 *			  psram_cs   PB01    psram_cs   PB27
 *			  psram_dat0 PB02    psram_dat0 PB02
 *			  psram_dat1 PB03    psram_dat1 PB03
 *			  psram_dat2 PB04    psram_dat2 PB04
 *			  psram_dat3 PB05    psram_dat3 PB05

 * @return None
 */
void wm_psram_config(uint8_t numsel);

/**
 * @brief  config the pins used for uart0 tx
 * @param  io_name: config uart0 tx pins name
 *			WM_IO_PB_19
 *			WM_IO_PB_27 only for 56pin
 *				
 * @return None
 */
void wm_uart0_tx_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for uart0 rx
 * @param  io_name: config uart0 rx pins name
 *			WM_IO_PB_20
 *				
 * @return None
 */
void wm_uart0_rx_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for uart1 tx
 * @param  io_name: config uart1 tx pins name
 *			WM_IO_PB_06
 *				
 * @return None
 */
void wm_uart1_tx_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for uart1 rx
 * @param  io_name: config uart1 rx pins name
 *			WM_IO_PB_07
 *			WM_IO_PB_16 only for 56pin
 *				
 * @return None
 */
void wm_uart1_rx_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for uart1 rts
 * @param  io_name: config uart1 rts pins name
 *			WM_IO_PB_19
 *			WM_IO_PA_02 only for 56pin 
 *				
 * @return None
 */
void wm_uart1_rts_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for uart1 cts
 * @param  io_name: config uart1 cts pins name
 *			WM_IO_PB_20
 *			WM_IO_PA_03 only for 56pin
 *				
 * @return None
 */
void wm_uart1_cts_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for uart2 tx or 7816-io
 * @param  io_name: config uart2 tx or 7816-io pins name
 *			WM_IO_PB_02
 *			WM_IO_PA_02 only for 56pin  
 *				
 * @return None
 */
void wm_uart2_tx_scio_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for uart2 rx
 * @param  io_name: config uart2 rx pins name
 *			WM_IO_PB_03
 *			WM_IO_PA_03 only for 56pin  
 *				
 * @return None
 */
void wm_uart2_rx_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for uart2 rts or 7816-clk
 * @param  io_name: config uart2 rts or 7816-clk pins name
 *			WM_IO_PB_04
 *			WM_IO_PA_05 only for 56pin  
 *				
 * @return None
 */
void wm_uart2_rts_scclk_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for uart2 cts
 * @param  io_name: config uart2 cts pins name
 *			WM_IO_PB_05
 *			WM_IO_PA_06 only for 56pin  
 *				
 * @return None
 */
void wm_uart2_cts_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for uart3 tx
 * @param  io_name: config uart1 tx pins name
 *			WM_IO_PB_00
 *			WM_IO_PA_05 only for 56pin  
 *				
 * @return None
 */
void wm_uart3_tx_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for uart3 rx
 * @param  io_name: config uart1 rx pins name
 *			WM_IO_PB_01
 *			WM_IO_PA_06 only for 56pin  
 *				
 * @return None
 */
void wm_uart3_rx_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for uart3 rts
 * @param  io_name: config uart3 rts pins name
 *			WM_IO_PA_02
 *				
 * @return None
 */
void wm_uart3_rts_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for uart3 cts
 * @param  io_name: config uart3 cts pins name
 *			WM_IO_PA_03
 *				
 * @return None
 */
 void wm_uart3_cts_config(enum tls_io_name io_name);


/**
 * @brief  config the pins used for uart4 tx
 * @param  io_name: config uart1 tx pins name
 *			WM_IO_PB_04
 *			WM_IO_PA_08 only for 56pin 
 *				
 * @return None
 */
void wm_uart4_tx_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for uart4 rx
 * @param  io_name: config uart1 rx pins name
 *			WM_IO_PB_05
 *			WM_IO_PA_09 only for 56pin  
 *				
 * @return None
 */
void wm_uart4_rx_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for uart4 rts
 * @param  io_name: config uart4 rts pins name
 *			WM_IO_PA_05 only for 56pin 
 *			WM_IO_PA_10 only for 56pin 
 *				
 * @return None
 */
void wm_uart4_rts_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for uart4 cts
 * @param  io_name: config uart4 cts pins name
 *			WM_IO_PA_06 only for 56pin 
 *			WM_IO_PA_11 only for 56pin 
 *				
 * @return None
 */
 void wm_uart4_cts_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for uart4 tx
 * @param  io_name: config uart1 tx pins name
 *			WM_IO_PA_08 only for 56pin 
 *			WM_IO_PA_12 only for 56pin 
 *			WM_IO_PB_18 only for 56pin 
 *				
 * @return None
 */
void wm_uart5_tx_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for uart4 rx
 * @param  io_name: config uart1 rx pins name
 *			WM_IO_PA_09 only for 56pin 
 *			WM_IO_PA_13 only for 56pin 
 *			WM_IO_PB_17 only for 56pin 
 *				
 * @return None
 */
void wm_uart5_rx_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for uart4 rts
 * @param  io_name: config uart4 rts pins name
 *			WM_IO_PA_14 only for 56pin 
 *			WM_IO_PB_12 only for 56pin 
 *				
 * @return None
 */
void wm_uart5_rts_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for uart4 cts
 * @param  io_name: config uart4 cts pins name
 *			WM_IO_PA_15 only for 56pin 
 *			WM_IO_PB_13 only for 56pin 
 *				
 * @return None
 */
 void wm_uart5_cts_config(enum tls_io_name io_name);



/**
 * @brief  config the pins used for i2s ck
 * @param  io_name: config i2s master ck pins name
 *			WM_IO_PA_04	 
 *			WM_IO_PB_08
 *			WM_IO_PA_08 only for 56pin 
 *			WM_IO_PB_12 only for 56pin 
 *				
 * @return None
 */
void wm_i2s_ck_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for i2s ws
 * @param  io_name: config i2s master ws pins name
 *			WM_IO_PA_01
 *			WM_IO_PB_09
 *			WM_IO_PA_09 only for 56pin 
 *			WM_IO_PB_13 only for 56pin  
 *				
 * @return None
 */
void wm_i2s_ws_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for i2s do
 * @param  io_name: config i2s master do pins name
 *			WM_IO_PA_00
 *			WM_IO_PB_11
 *			WM_IO_PA_10 only for 56pin 
 *			WM_IO_PB_14 only for 56pin   
 *				
 * @return None
 */
void wm_i2s_do_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for i2s di
 * @param  io_name: config i2s slave di pins name
 *			WM_IO_PA_07
 *			WM_IO_PB_10
 *			WM_IO_PA_11 only for 56pin 
 *			WM_IO_PB_15 only for 56pin   
 *				
 * @return None
 */
void wm_i2s_di_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for i2s mclk
 * @param  io_name: config i2s mclk pins name
 *			WM_IO_PA_00
 *				
 * @return None
 */
void wm_i2s_mclk_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for i2s extclk
 * @param  io_name: config i2s extclk pins name
 *			WM_IO_PA_07
 *				
 * @return None
 */
void wm_i2s_extclk_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for i2c scl
 * @param  io_name: config i2c scl pins name
 *			WM_IO_PA_01
 *			WM_IO_PB_20
 *				
 * @return None
 */
void wm_i2c_scl_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for i2c sda
 * @param  io_name: config i2c sda pins name
 *			WM_IO_PA_04
 *			WM_IO_PB_19
 *				
 * @return None
 */
void wm_i2c_sda_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for pwm0
 * @param  io_name: config pwm1 pins name
 *			WM_IO_PB_00
 *			WM_IO_PB_19
 *			WM_IO_PA_02 only for 56pin 
 *			WM_IO_PA_10 only for 56pin   
 *			WM_IO_PB_12 only for 56pin   
 *				
 * @return None
 */
void wm_pwm0_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for pwm1
 * @param  io_name: config pwm1 pins name
 *			WM_IO_PB_01
 *			WM_IO_PB_20
 *			WM_IO_PA_03 only for 56pin 
 *			WM_IO_PA_11 only for 56pin   
 *			WM_IO_PB_13 only for 56pin    
 *				
 * @return None
 */
void wm_pwm1_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for pwm2
 * @param  io_name: config pwm3 pins name
 *			WM_IO_PA_00
 *			WM_IO_PB_02 
 *			WM_IO_PA_12 only for 56pin 
 *			WM_IO_PB_14 only for 56pin   
 *			WM_IO_PB_24 only for 56pin    
 *				
 * @return None
 */
void wm_pwm2_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for pwm3
 * @param  io_name: config pwm4 pins name
 *			WM_IO_PA_01
 *			WM_IO_PB_03
 *			WM_IO_PA_13 only for 56pin 
 *			WM_IO_PB_15 only for 56pin   
 *			WM_IO_PB_25 only for 56pin     
 *				
 * @return None
 */
void wm_pwm3_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for pwm4
 * @param  io_name: config pwm5 pins name
 *			WM_IO_PA_04
 *			WM_IO_PA_07 
 *			WM_IO_PA_14 only for 56pin 
 *			WM_IO_PB_16 only for 56pin   
 *			WM_IO_PB_26 only for 56pin     
 *				
 * @return None
 */
void wm_pwm4_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for pwm break
 * @param  io_name: config pwm break pins name
 *			WM_IO_PB_08
 *			WM_IO_PA_05 only for 56pin 
 *			WM_IO_PA_08 only for 56pin   
 *			WM_IO_PA_15 only for 56pin 
 *			WM_IO_PB_17 only for 56pin     
 *				
 * @return None
 */
void wm_pwmbrk_config(enum tls_io_name io_name);

/**
 * @brief  config the pins used for swd
 * @param  enable: enable or disable chip swd function
 *			1: enable
 *			0: disable
 *				
 * @return None
 */
void wm_swd_config(bool enable);

/**
 * @brief  config the pins used for adc
 * @param  Channel: the channel that shall be used
 *			0~1: single-ended input
 *			2~3: single-ended input only for 56pin
 *			0 and 1 can be used differential input
 *			2 and 3 can be used differential input only for 56pin
 *				
 * @return None
 */
void wm_adc_config(u8 Channel);

/**
 * @brief  config the pins used for touch sensor
 * @param  io_name: config touch sensor pins name
 *			WM_IO_PA_07
 *			WM_IO_PB_00
 *			WM_IO_PB_01
 *			WM_IO_PB_02
 *			WM_IO_PB_03
 *			WM_IO_PB_04
 *			WM_IO_PB_05
 *			WM_IO_PB_06
 *			WM_IO_PB_07
 *			WM_IO_PB_08
 *			WM_IO_PB_09
 *			WM_IO_PA_09 only for 56pin
 *			WM_IO_PA_10 only for 56pin
 *			WM_IO_PA_12 only for 56pin
 *			WM_IO_PA_14 only for 56pin
 *				
 * @return None
 * @note  If user use touch sensor function, firstly consider using WM_IO_PA_07 as TOUCH SENSOR pin.
 */
 void wm_touch_sensor_config(enum tls_io_name io_name);

/**
 * @brief  disable all the gpio af
 *				
 * @return None
 *
 * @note  This function must call before any others for configure 
 * 		  gpio Alternate functions
 */
void wm_gpio_af_disable(void);
/**
 * @}
 */

/**
 * @}
 */

#endif /* end of WM_GPIO_AFSEL_H */

