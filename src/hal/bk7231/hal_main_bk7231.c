
#include "../../new_common.h"
#include "../../logging/logging.h"


#include "../../beken378/app/config/param_config.h"


// main timer tick every 1s
beken_timer_t g_main_timer_1s;
// from rtos_pub.c
extern uint32_t  ms_to_tick_ratio;


// a bitfield indicating which GPI are inputs.
// could be used to control edge triggered interrupts...
extern uint32_t g_gpi_active;


#if PLATFORM_BK7231T

// realloc fix - otherwise calling realloc crashes.
// Just fall back to os_realloc.
_PTR realloc _PARAMS ((_PTR a, size_t b)) {
	return os_realloc(a,b);
}

#endif

#define LOG_FEATURE LOG_FEATURE_MAIN
extern int MQTT_process_received();
void MQTT_process_received_timer(void *a, void*b){
  MQTT_process_received();
}



#if ( INCLUDE_xTimerPendFunctionCall == 1 )
  // note, if called from an ISR, delay is ignored
  // this function allows a function to be queued on the timer thread.
  // initially this seemed attractive, but using a oneshot timer is probably better,
  // so currently unused

  // HMM... the delay here is max time to wait for queue space.
  // NOT the delay before the function is called...!!!
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
#endif


// allow us to control if it is used or not
#undef OBK_REQUEST_PENDFUNCTION

#ifdef OBK_REQUEST_PENDFUNCTION
  #if ( INCLUDE_xTimerPendFunctionCall == 1 )
    #define OBK_USE_PENDFUNCTION
  #endif
#else
#endif


// bitfield values for g_timer_triggers
#define ONESHOT_MQTT_PROCESS 1
#define ONESHOT_BUTTON_PROCESS 2

// from new_pins.c
extern void PIN_ticks(void *param);

// we use this if FreeRTOS is configured for xTimerPendFunctionCall
// with #define INCLUDE_xTimerPendFunctionCall 1 in FreeRTOSConfig.h

// our timer thread callback - extend for multiple options,
// e.g. pins called back from GPIO level interrupt.
void OBK_timer_cb(void *a, uint32_t cmd){
  switch (cmd){
    case ONESHOT_MQTT_PROCESS:
      MQTT_process_received();
      break;
    case ONESHOT_BUTTON_PROCESS:
      //MQTT_process_received();
      // 0 means it's a poll
      trigger_50ms_oneshot(ONESHOT_BUTTON_PROCESS);
      break;
  }
}

// defer to a DIFFERENT function in timer thread, else we can't set the timer from within itself...
void OBK_TriggerButtonPoll(){
  OSStatus err;
  err = OBK_rtos_callback_in_timer_thread( OBK_timer_cb, NULL, ONESHOT_BUTTON_PROCESS, 100);
}


// this is a bitfield for oneshot timer requests
uint32_t g_timer_triggers = 0;
uint32_t g_50ms_timer_triggers = 0;

void process_oneshot_timer(void *a, void*b){
  // if mqtt process incomming data requested
  if (g_timer_triggers & ONESHOT_MQTT_PROCESS){
    MQTT_process_received();
    g_timer_triggers &= ~ONESHOT_MQTT_PROCESS;
  }

  if (g_timer_triggers & ONESHOT_BUTTON_PROCESS){
    //MQTT_process_received();
    // 1 means it's a trigger/interrupt
    PIN_ticks((void *)1);
    g_timer_triggers &= ~ONESHOT_BUTTON_PROCESS;
  }
}

void process_50ms_oneshot_timer(void *a, void*b){
  if (g_50ms_timer_triggers & ONESHOT_BUTTON_PROCESS){
    //MQTT_process_received();
    // 1 means it's a trigger/interrupt
    PIN_ticks((void *)0);
    g_50ms_timer_triggers &= ~ONESHOT_BUTTON_PROCESS;
  }
}

// we use this only if FreeRTOS is not configured for xTimerPendFunctionCall
// with #define INCLUDE_xTimerPendFunctionCall 1 in FreeRTOSConfig.h
beken2_timer_t g_timer_oneshot;
beken2_timer_t g_50ms_timer_oneshot;

void trigger_oneshot(uint32_t type){
  OSStatus err;
  // note that we watn MQTT input to be processed
  g_timer_triggers |= type;
  // will start or reset the oneshot timer.
  // the timer should fire 1ms after this call.
  // if two calls are made before the timer fires, it will fire ONCE
  // 1ms after the last call....
  // the timer calls process_oneshot_timer
  err = rtos_start_oneshot_timer(&g_timer_oneshot);
}

void trigger_50ms_oneshot(uint32_t type){
  OSStatus err;
  int fire = 1;
  if (g_50ms_timer_triggers) fire = 0;
  // note that we watn MQTT input to be processed
  g_50ms_timer_triggers |= type;
  // will start or reset the oneshot timer.
  // the timer should fire 1ms after this call.
  // if two calls are made before the timer fires, it will fire ONCE
  // 1ms after the last call....
  // the timer calls process_oneshot_timer
  if (fire) err = rtos_start_oneshot_timer(&g_50ms_timer_oneshot);
}

// these functions should (are) safe to call from ISR or task.
void MQTT_TriggerRead(){
  trigger_oneshot(ONESHOT_MQTT_PROCESS);
}
void BUTTON_TriggerRead(){
  trigger_oneshot(ONESHOT_BUTTON_PROCESS);
}

void BUTTON_TriggerRead_50ms(){
  OBK_TriggerButtonPoll();
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
	ADDLOGF_DEBUG("started timer");

  // initialise a one-shot timer, triggered by MQTT_TriggerRead()
  err = rtos_init_oneshot_timer(&g_timer_oneshot,
                        1,
                        process_oneshot_timer,
                        (void *)0,
                        (void *)0);
  ASSERT(kNoErr == err);

  err = rtos_init_oneshot_timer(&g_50ms_timer_oneshot,
                        50,
                        process_50ms_oneshot_timer,
                        (void *)0,
                        (void *)0);
  ASSERT(kNoErr == err);

	ADDLOGF_DEBUG("initialised oneshot timers");
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


