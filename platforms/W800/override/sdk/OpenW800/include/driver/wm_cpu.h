/**
 * @file    wm_cpu.h
 *
 * @brief   cpu driver module
 *
 * @author  dave
 *
 * @copyright (c) 2014 Winner Microelectronics Co., Ltd.
 */
#ifndef WM_CPU_H
#define WM_CPU_H

#include <stdint.h>

/**W800 BASE PLL CLOCK*/
#define W800_PLL_CLK_MHZ  		(480)

enum CPU_CLK{
	CPU_CLK_240M = 2,
	CPU_CLK_160M = 3,
	CPU_CLK_80M  = 6,
	CPU_CLK_40M  = 12,
	CPU_CLK_2M  = 240,		
};

typedef union {
    struct {
        uint32_t CPU: 8;                     /*!< bit:  0.. 7  cpu clock divider */
        uint32_t WLAN: 8;                    /*!< bit:  8.. 15 Wlan clock divider */
        uint32_t BUS2: 8;                    /*!< bit:  16.. 23 clock dividing ratio of bus2 & bus1 */
        uint32_t PD: 4;                      /*!< bit:  24.. 27  peripheral divider */
        uint32_t RSV: 3;                     /*!< bit:  28.. 30  Reserved */
        uint32_t DIV_EN: 1;                  /*!< bit:  31     divide frequency enable */
    } b;
    uint32_t w;
} clk_div_reg;

#define UNIT_MHZ		(1000000)


typedef struct{
	u32 apbclk;
	u32 cpuclk;
	u32 wlanclk;
}tls_sys_clk;

/**
 * @defgroup Driver_APIs Driver APIs
 * @brief Driver APIs
 */

/**
 * @addtogroup Driver_APIs
 * @{
 */

/**
 * @defgroup CPUCLK_Driver_APIs CPU CLOCK Driver APIs
 * @brief CPU CLOCK driver APIs
 */

/**
 * @addtogroup CPUCLK_Driver_APIs
 * @{
 */


/**
 * @brief          This function is used to set cpu clock
 *
 * @param[in]      	clk    select cpu clock
 *                        	clk == CPU_CLK_80M	80M
 *				clk == CPU_CLK_40M	40M
 *
 * @return         None
 *
 * @note           None
 */
void tls_sys_clk_set(u32 clk);


/**
 * @brief          	This function is used to get cpu clock
 *
 * @param[out]     *sysclk	point to the addr for system clk output
 *
 * @return         	None
 *
 * @note           	None
 */
void tls_sys_clk_get(tls_sys_clk *sysclk);

/**
 * @}
 */

/**
 * @}
 */

#endif /* WM_CPU_H */
