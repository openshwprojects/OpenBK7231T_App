/**
 * @file    wm_touchsensor.h
 *
 * @brief   touchsensor Driver Module
 *
 * @author  
 *
 * Copyright (c) 2021 Winner Microelectronics Co., Ltd.
 */
#include "wm_type_def.h"

/**
 * @brief          This function is used to initialize touch sensor.
 *
 * @param[in]      sensorno    is the touch sensor number from 1-15
 * @param[in]      scan_period is scan period for per touch sensor ,unit:16ms, >0
 * @param[in]      window      is count window, window must be greater than 2.Real count window is window - 2.
 * @param[in]      enable      is touch sensor enable bit.
 *
 * @retval         0:success
 *
 * @note           if use touch sensor, user must configure the IO multiplex by API wm_touch_sensor_config.
 */
int tls_touchsensor_init_config(u32 sensorno, u8 scan_period, u8 window, u32 enable);

/**
 * @brief          This function is used to initialize touch scan channel.
 *
 * @param[in]      sensorno    is the touch sensor number from 1-15
 *
 * @retval         0:success
 *
 * @note           if use touch sensor, user must configure the IO multiplex by API wm_touch_sensor_config.
 */
int tls_touchsensor_chan_config(u32 sensorno);

/**
 * @brief          This function is used to initialize touch general configuration.
 *
 * @param[in]      scanperiod  is scan period for per touch sensor ,unit:16ms, >0
 * @param[in]      window      is count window, window must be greater than 2.Real count window is window - 2.
 * @param[in]      bias        is touch sensor bias current
 *
 * @retval         0:success
 *
 * @note           if use touch sensor, user must configure the IO multiplex by API wm_touch_sensor_config.
 */
int tls_touchsensor_scan_config(u8 scanperiod, u8 window, u8 bias);

/**
 * @brief          This function is used to start touch scan
 *
 * @retval         0:success
 *
 * @note           if use touch sensor, user must configure the IO multiplex by API wm_touch_sensor_config.
 */
int tls_touchsensor_scan_start(void);

/**
 * @brief          This function is used to stop touch scan
 *
 * @retval         0:success
 *
 * @note           if use touch sensor, user must configure the IO multiplex by API wm_touch_sensor_config.
 */
int tls_touchsensor_scan_stop(void);


/**
 * @brief          This function is used to deinit touch sensor's selection and disable touch.
 *
 * @param[in]      sensorno    is the touch sensor number from 1-15
 *
 * @retval         0:success
 *
 * @note           if do not use touch sensor, user can deinit by this interface and configure this touch sensor as GPIO.
 */
int tls_touchsensor_deinit(u32 sensorno);


/**
 * @brief          This function is used to set threshold per touch sensor.
 *
 * @param[in]      sensorno    is the touch sensor number from 1-15
 * @param[in]      threshold   is the sensorno's touch sensor threshold,max value is 127.
 *
 * @retval         0:success. minus value: parameter wrong.
 *
 * @note           None
 */
int tls_touchsensor_threshold_config(u32 sensorno, u8 threshold);


/**
 * @brief          This function is used to get touch sensor's count number.
 *
 * @param[in]      sensorno    is the touch sensor number from 1 to 15.
 *
 * @retval         sensorno's count number  .
 *
 * @note           None
 */
int tls_touchsensor_countnum_get(u32 sensorno);

/**
 * @brief          This function is used to enable touch sensor's irq.
 *
 * @param[in]      sensorno    is the touch sensor number  from 1 to 15.
 *
 * @retval         0:successfully enable irq, -1:parameter wrong.
 *
 * @note           None
 */
int tls_touchsensor_irq_enable(u32 sensorno);

/**
 * @brief          This function is used to disable touch sensor's irq.
 *
 * @param[in]      sensorno    is the touch sensor number  from 1 to 15.
 *
 * @retval         0:successfully disable irq, -1:parameter wrong.
 *
 * @note           None
 */
int tls_touchsensor_irq_disable(u32 sensorno);

/**
 * @brief          This function is used to register touch sensor's irq callback.
 *
 * @param[in]      callback    is call back for user's application.
 *
 * @retval         None.
 *
 * @note           None
 */
void tls_touchsensor_irq_register(void (*callback)(u32 status));

/**
 * @brief          This function is used to get touch sensor's irq status.
 *
 * @param[in]      sensorno    is the touch sensor number  from 1 to 15.
 *
 * @retval         >=0:irq status, -1:parameter wrong.
 *
 * @note           None
 */
int tls_touchsensor_irq_status_get(u32 sensorno);



