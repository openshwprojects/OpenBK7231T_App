
#include "../../new_common.h"
#include "../../logging/logging.h"


#include "../../beken378/app/config/param_config.h"


// main timer tick every 1s
beken_timer_t g_main_timer_1s;
beken2_timer_t g_mqtt_timer_oneshot;


#if PLATFORM_BK7231T

// realloc fix - otherwise calling realloc crashes.
// Just fall back to os_realloc.
_PTR realloc _PARAMS ((_PTR a, size_t b)) {
	return os_realloc(a,b);
}

#endif

#define LOG_FEATURE LOG_FEATURE_MAIN

// from rtos_pub.c
extern uint32_t  ms_to_tick_ratio;

// note, if called from an ISR, delay is ignored
// this function allows a function to be queued on the timer thread.
OSStatus OBK_rtos_callback_in_timer_thread( PendedFunction_t xFunctionToPend, void *pvParameter1, uint32_t ulParameter2, uint32_t delay_ms)
{
  signed portBASE_TYPE result;

  if ( platform_is_in_interrupt_context() == RTOS_SUCCESS ) {
    signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    result = xTimerPendFunctionCallFromISR( xFunctionToPend, pvParameter1, ulParameter2, &xHigherPriorityTaskWoken );
    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
  } 
  else
  {
    result = xTimerPendFunctionCall( xFunctionToPend, pvParameter1, ulParameter2, ( delay_ms / ms_to_tick_ratio ));
  }

  if ( result != pdPASS )
  {
      return kGeneralErr;
  }

  return kNoErr;
}

extern int MQTT_process_received();
void MQTT_process_received_timer(void *a, void*b){
  MQTT_process_received();
}

void MQTT_TriggerRead(){
  OSStatus err;
  err = rtos_start_oneshot_timer(&g_mqtt_timer_oneshot);
}


void user_main(void)
{
    OSStatus err;
	Main_Init();


  err = rtos_init_timer(&g_main_timer_1s,
                        1 * 1000,
                        Main_OnEverySecond,
                        (void *)0);
  ASSERT(kNoErr == err);

  err = rtos_start_timer(&g_main_timer_1s);
  ASSERT(kNoErr == err);
	ADDLOGF_DEBUG("started timer\r\n");

  // initialise a one-shot timer, triggered by MQTT_TriggerRead()
  err = rtos_init_oneshot_timer(&g_mqtt_timer_oneshot,
                        1,
                        MQTT_process_received_timer,
                        (void *)0,
                        (void *)0);
}

#if PLATFORM_BK7231N

// right now Free is somewhere else

#else

#undef Free
// This is needed by tuya_hal_wifi_release_ap.
// How come that the Malloc was not undefined, but Free is?
// That's because Free is defined to os_free. It would be better to fix it elsewhere
void Free(void* ptr)
{
    os_free(ptr);
}


#endif


