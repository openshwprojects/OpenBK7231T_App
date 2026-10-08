#ifdef WINDOWS

#include "../hal_os_wrapper.h"

/** @brief Get free heap size from OS
  *
  * @return    none
  */
uint32_t obk_get_free_heap_size( void ) {
	return -1;
}

/** @brief Enter a critical session, all interrupts are disabled
  *
  * @return    none
  */
void obk_enter_critical( void )
{

}
/** @brief Exit a critical session, all interrupts are enabled
  *
  * @return    none
  */
void obk_exit_critical( void )
{

}
/** @brief OS delay
  *
  * @return    none
  */
void obk_delay_ms( uint32_t delay ) {	

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