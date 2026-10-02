#if PLATFORM_ESP8266

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../hal_os_wrapper.h"
#include "esp_system.h"


/** @brief Get free heap size from OS
  *
  * @return    none
  */
uint32_t obk_get_free_heap_size( void ) {
	return esp_get_minimum_free_heap_size();
}

/** @brief Enter a critical session, all interrupts are disabled
  *
  * @return    none
  */
void obk_enter_critical( void )
{
	taskENTER_CRITICAL();
}
/** @brief Exit a critical session, all interrupts are enabled
  *
  * @return    none
  */
void obk_exit_critical( void )
{
	taskEXIT_CRITICAL();
}
/** @brief OS delay
  *
  * @return    none
  */
void obk_delay_ms( uint32_t delay ) {	
    uint32_t ticks;

    ticks = delay / portTICK_PERIOD_MS;
    if (ticks == 0)
        ticks = 1;

    vTaskDelay( (portTickType) ticks );
}
/** @defgroup OS Mutex Functions Wrappers
  * @brief Provide management APIs for Mutex such as init,lock,unlock and dinit.
  * @{
  */

/** @brief    Initialises a mutex
  *
  * @Details  A mutex is different to a semaphore in that a thread that already holds
  *           the lock on the mutex can request the lock again (nested) without causing
  *           it to be suspended.
  *
  * @param    mutex : a pointer to the mutex handle to be initialised
  *
  * @return   OBK_EOK        : on success.
  * @return   OBK_ERROR      : if an error occurred
  */
obk_err_t obk_init_mutex( obk_mutex_t* mutex )
{
	if (!mutex) return OBK_ERROR;
	*(xSemaphoreHandle*) mutex = xSemaphoreCreateMutex();
	return OBK_EOK;
}
/** @brief    Obtains the lock on a mutex
  *
  * @Details  Attempts to obtain the lock on a mutex. If the lock is already held
  *           by another thead, the calling thread will be suspended until the mutex 
  *           lock is released by the other thread.
  *
  * @param    mutex : a pointer to the mutex handle to be locked
  *
  * @return   OBK_EOK        : on success.
  * @return   OBK_ERROR      : if an error occurred
  */
obk_err_t obk_lock_mutex( obk_mutex_t* mutex, uint32_t timeout_ms)
{	
	return OBK_EOK;
    if (!mutex || !(*mutex)) return OBK_ERROR;
	TickType_t xTicksToWait = portMAX_DELAY;
	if (timeout_ms != 0) {
		xTicksToWait = timeout_ms / portTICK_PERIOD_MS;
	}			
    // Try lock mutex
    if (xSemaphoreTake(*(xSemaphoreHandle*)mutex, xTicksToWait) == pdTRUE) {
		return OBK_EOK;
	} else return OBK_ERROR;
}
/** @brief    Releases the lock on a mutex
  *
  * @Details  Releases a currently held lock on a mutex. If another thread
  *           is waiting on the mutex lock, then it will be resumed.
  *
  * @param    mutex : a pointer to the mutex handle to be unlocked
  *
  * @return   OBK_EOK        : on success.
  * @return   OBK_ERROR      : if an error occurred
  */
obk_err_t obk_unlock_mutex( obk_mutex_t* mutex )
{	
	return OBK_EOK;
	if (!mutex || !(*mutex)) return OBK_ERROR;
	xSemaphoreGive(*(xSemaphoreHandle*)mutex);
	return OBK_EOK;
}


/** @brief    De-initialise a mutex
  *
  * @Details  Deletes a mutex created with @ref rtos_init_mutex
  *
  * @param    mutex : a pointer to the mutex handle
  *
  * @return   OBK_EOK        : on success.
  * @return   OBK_ERROR   : if an error occurred
  */
obk_err_t obk_deinit_mutex( obk_mutex_t* mutex )
{
	return OBK_EOK;
}
/**
  * @}
  */

#endif