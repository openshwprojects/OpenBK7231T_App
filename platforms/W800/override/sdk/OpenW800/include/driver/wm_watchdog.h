/**
 * @file    wm_watchdog.h
 *
 * @brief   watchdog Driver Module
 *
 * @author  dave
 *
 * Copyright (c) 2014 Winner Microelectronics Co., Ltd.
 */
#ifndef WM_WATCHDOG_H
#define WM_WATCHDOG_H

/**
 * @defgroup Driver_APIs Driver APIs
 * @brief Driver APIs
 */

/**
 * @addtogroup Driver_APIs
 * @{
 */

/**
 * @defgroup WDG_Driver_APIs WDG Driver APIs
 * @brief WDG driver APIs
 */

/**
 * @addtogroup WDG_Driver_APIs
 * @{
 */

/**
 * @brief          This function is used to feed the dog.
 *
 * @param          None
 *
 * @return         None
 *
 * @note           None
 */
void tls_watchdog_clr(void);

/**
 * @brief          This function is used to init and start the watchdog.
 *
 * @param[in]      usec    microseconds
 *
 * @return         None
 *
 * @note           None
 */
void tls_watchdog_init(u32 usec);

/**
 * @brief          This function is used to deinit watchdog
 *
 * @param[in]     None
 *
 * @return         None
 *
 * @note           None
 */
void tls_watchdog_deinit(void);

/**
 * @brief          This function is used to reset the system.
 *
 * @param          None
 *
 * @return         None
 *
 * @note           None
 */
void tls_sys_reset(void);

/**
 * @brief          This function is used to set reboot reason
 *
 * @param          reason (enum SYS_REBOOT_REASON)
 *
 * @return         None
 *
 * @note           used with tls_sys_reset
 */
void tls_sys_set_reboot_reason(u32 reason);

/**
 * @brief          This function is used to get reboot reason
 *
 * @param          None
 *
 * @return         reason (enum SYS_REBOOT_REASON)
 *
 * @note           None
 */
int tls_sys_get_reboot_reason(void);


/**
 * @}
 */

/**
 * @}
 */

#endif /* WM_WATCHDOG_H */

