
#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../quicktick.h"


#include "../../beken378/app/config/param_config.h"


// main timer tick every 1s
beken_timer_t g_main_timer_1s;
// 1ms oneshot timer - effectively a pended function, but overridable.
beken2_timer_t g_timer_oneshot;
// oneshot timer at quicktick interval
beken2_timer_t g_quick_timer_oneshot;

// fwd declaration
void trigger_quick_oneshot(uint32_t type);

// from new_mqtt.c
extern int MQTT_process_received();
// from new_pins.c
extern void PIN_ticks(void *param);

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


// bitfield values for g_timer_triggers
#define ONESHOT_MQTT_PROCESS 1
#define ONESHOT_BUTTON_PROCESS 2

// this is a bitfield for oneshot timer requests
uint32_t g_timer_triggers = 0;
uint32_t g_quick_timer_triggers = 0;



// OBK_timer_cb cmd values
#define OBK_DEFER_BUTTON_POLL 0

// our timer thread callback - extend for multiple options,
// e.g. pins called back from GPIO level interrupt.
// this function is PENDED below.
void OBK_timer_cb(void *a, uint32_t cmd){
  switch (cmd){
    case OBK_DEFER_BUTTON_POLL:
      trigger_quick_oneshot(ONESHOT_BUTTON_PROCESS);
      break;
  }
}

// NOTE: defer to a DIFFERENT function in timer thread, 
// else **** we can't set the timer from within itself... ****
void OBK_TriggerButtonPoll(){
  OSStatus err;
  // note the 100 is ms to wait if timer queue full, NOT delay to function call.
  err = OBK_rtos_callback_in_timer_thread( OBK_timer_cb, NULL, OBK_DEFER_BUTTON_POLL, 100);
}


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

void process_quick_oneshot_timer(void *a, void*b){
  if (g_quick_timer_triggers & ONESHOT_BUTTON_PROCESS){
    //MQTT_process_received();
    // 1 means it's a trigger/interrupt
    PIN_ticks((void *)0);
    g_quick_timer_triggers &= ~ONESHOT_BUTTON_PROCESS;
  }
}


// NOTE: this one RESETS the timer if the timer is pending.
// so many calls will result in delayed firing.
void trigger_oneshot(uint32_t type){
  OSStatus err;
  // note that we want (type) to be processed
  g_timer_triggers |= type;
  // will start or reset the oneshot timer.
  // the timer should fire 1ms after this call.
  // if two calls are made before the timer fires, it will fire ONCE
  // 1ms after the last call....
  // the timer calls process_oneshot_timer
  err = rtos_start_oneshot_timer(&g_timer_oneshot);
}

// NOTE: this one only triggers the timer if it's not already pending.
// so many calls will result undelayed firing.
// i.e. the timer will fire 50ms after the FIRST call when the timer is not pending
void trigger_quick_oneshot(uint32_t type){
  OSStatus err;
  int fire = 1;
  if (g_quick_timer_triggers) fire = 0;
  // note that we watn MQTT input to be processed
  g_quick_timer_triggers |= type;
  // will start or reset the oneshot timer.
  // the timer should fire 1ms after this call.
  // if two calls are made before the timer fires, it will fire ONCE
  // 1ms after the last call....
  // the timer calls process_oneshot_timer
  if (fire) err = rtos_start_oneshot_timer(&g_quick_timer_oneshot);
}


// these functions should (are) safe to call from ISR or task.
// they use the above functions

// basically, 'read mqtt as soon as possible'
void MQTT_TriggerRead(){
  trigger_oneshot(ONESHOT_MQTT_PROCESS);
}

// basically, 'read buttons as soon as possible'
void BUTTON_TriggerRead(){
  trigger_oneshot(ONESHOT_BUTTON_PROCESS);
}

// basically, 'read buttons in quick tick period' - ~25ms
void BUTTON_TriggerRead_quick(){
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
	ADDLOGF_DEBUG("started timer\r\n");

  // initialise a one-shot timer, triggered by MQTT_TriggerRead() and others
  err = rtos_init_oneshot_timer(&g_timer_oneshot,
                        1,
                        process_oneshot_timer,
                        (void *)0,
                        (void *)0);
  ASSERT(kNoErr == err);

  err = rtos_init_oneshot_timer(&g_quick_timer_oneshot,
                        QUICK_TMR_DURATION,
                        process_quick_oneshot_timer,
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


