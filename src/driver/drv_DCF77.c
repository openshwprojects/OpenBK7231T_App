#include "drv_DCF77.h"
#include "../obk_config.h"

#if (ENABLE_DRIVER_DCF77)

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "../cmnds/cmd_public.h"
#include "../hal/hal_pins.h"
#include "../logging/logging.h"
#include "../new_cfg.h"
#include "../new_pins.h"
#include "../libraries/obktime/obktime.h"	// for time functions
#include "drv_deviceclock.h"


#if PLATFORM_BEKEN
#include "BkDriverTimer.h"
#include "BkDriverGpio.h"
#include "sys_timer.h"
#include "gw_intf.h"
#elif PLATFORM_W600 || PLATFORM_W800
#include "wm_gpio.h"
#define IRQ_raise_and_fall 1
#elif PLATFORM_BL602
#include "hal_gpio.h"
#include "bl_gpio.h"
#elif PLATFORM_LN882H
#include "../../sdk/OpenLN882H/mcu/driver_ln882h/hal/hal_common.h"
#include "../../sdk/OpenLN882H/mcu/driver_ln882h/hal/hal_gpio.h"
#define IRQ_raise_and_fall 1
#elif PLATFORM_REALTEK
#include "gpio_irq_api.h"
#include "../hal/realtek/hal_pinmap_realtek.h"
rtlPinMapping_t* rtl_dcf77;
#elif PLATFORM_ECR6600
#include "gpio.h"
#elif PLATFORM_XRADIO
#include "../hal/xradio/hal_pinmap_xradio.h"
extern void HAL_XR_ConfigurePin(GPIO_Port port, GPIO_Pin pin, GPIO_WorkMode mode, GPIO_PullType pull);
xrpin_t* xr_dcf77;
#elif PLATFORM_ESP8266 || PLATFORM_ESPIDF
#include "../hal/espidf/hal_pinmap_espidf.h"
espPinMapping_t* esp_dcf77;
#define IRQ_raise_and_fall 1
#endif

static uint32_t lastset; // secondsElapsed on last successfull reading of time

static uint8_t settime=0;	// use to signal state outside ISR: 0: still collecting bits, 1: 59 bits collected (but not decoded), 2: time successfull decoded, 3: decoding failed, 4: bits for summer-/winter-time not valid

static bool httpstat = false;	// per default don't add status to main page - only use "clock" output. can be set during driver start	

int GPIO_DCF77 = -1;
#if PLATFORM_W600 || PLATFORM_W800
unsigned int GPIO_DCF77_pin;
#endif

#define DCF77_SYNC_THRESHOLD_MS 1500 // missing pulse threshold (between bits ~1000ms, missing pulse ~2000ms)
#define DCF77_MIN_PULSE_MS 20
#define DCF77_MAX_PULSE_MS 400

typedef enum {
	RISING = 0,
	FALLING = 1
} IRQ_level;

// Default, might be changed by adding "fall" to startDriver DCF77 if falling edge indicates start of bit (e.g. if signal is inverted by optocoupler)  
uint8_t STARTEDGE = RISING;

static uint8_t next_IRQ;
// define max duration for "0" bit and min duration of "1" bit - might be overridden by command line 
static uint8_t end0=180;
static uint8_t start1=180;

#if PLATFORM_BEKEN || PLATFORM_BL602
// will be used to "toggle" between the edges
uint8_t TRIGGER_START, TRIGGER_END;
#else
#endif


static volatile uint32_t dcf77_last_edge_tick = 0;
static volatile uint32_t dcf77_pulse_length_ms = 0;
static volatile uint8_t dcf77_minute_bits[59];
static volatile uint8_t dcf77_pulse_count = 0;
static volatile bool dcf77_synced = false;
static volatile time_t dcf77_next_minute_epoch = 0;
//static volatile bool dcf77_has_valid_epoch = false;

static uint32_t get_ms_tick() {
#ifdef PLATFORM_W600
	typedef portTickType TickType_t;		// W600/W800: xTaskGetTickCount() is of type "portTickType", all others "TickType_t" , W600 has no definition for TickType_t
#endif
#if PLATFORM_BEKEN || PLATFORM_LN882H || PLATFORM_REALTEK || PLATFORM_XRADIO || PLATFORM_ESP8266 || PLATFORM_ESPIDF || PLATFORM_W600 || PLATFORM_W800
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
#elif 0
    portTICK_RATE_MS * xTaskGetTickCount();
#elif PLATFORM_BL602 || PLATFORM_ECR6600
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
#else
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
#endif
}

#if PLATFORM_BL602 || PLATFORM_BEKEN
static void DCF77_Interrupt(void* arg);
#endif



int minute;
int hour;
int day;
int weekday;
int month;
int year;
int part_par_date;	// well start and calc parity up to day, so only year is open in ISR
int partdecoded=0;
//struct tm t;
//TimeComponents tc;
int pre_valid=0;
// DCF77 epoch calculation

// Helper: convert BCD bits to int
// we have bits in 64 bit uint
//
//      9876543210
// .....xxxxxxxxxx
//
//	LSB is bit 0
//
static int u64_to_bcd(const uint64_t bits, int pos, int count) {
    int val = 0;
    // right shift "pos", so wanted bits start at LSB
    // mask out all but last "count" bits
    uint64_t x = (bits >> pos) & ((1ULL  << count) - 1);
    val = (int) x - (( x >> 3) & 30) * 3 ;
    // factors are: 1,2,4,8,10,20,40,80
    //  we'll use some "magic" to calc with standard 1,2,4,8,16,32,64,128
    //
    // what happens?
    // instead of 10 we use 16	 6 to much
    // instead of 20 we use 32	12 to much
    // instead of 40 we use 64	24 to much
    // instead of 80 we use 128	48 to much
    // you see the logic? it's, depending on the position 6, 2*6, 4*6, 8*6 to much
    // or: 2*3, 4*3, 8*3, 16*3
    // so we use the "regular" integer and substract the "overhead" which is
    // related to the bits 8 (48 to much, if 1), 7 (24 to much, if 1) 6 (12 to much, if 1) and 5 (6 to much, if 1)
    // in bits:		(bits >> 4 & 15) * 6 = (bits >> 3 & 30) * 3  ( to be sure, ad a mask 00001111 (15) for >>4   or 00011110 (30) for >>3)
    return val;
}

static int u64_to_parity(const uint64_t bits, int pos, int count) {
    int p = 0;
    // right shift "pos", so wanted bits start at LSB
    // mask out all but last "count" bits
    uint64_t x = (bits >> pos) & ((1ULL << count) - 1);
    // count 1 (Brian Kernighan's algorithm)
    while (x) {
        x &= (x - 1); // Clear the least significant 1 bit
        p++;
    }
    return p%2;
}


// DCF77 epoch calculation
static void raw_dcf77_decode_u64(const uint64_t bits, int *minute, int *hour, int *day, int *weekday, int *month, int *year) {
    // Minute: bits 21-27 (7)
    *minute = u64_to_bcd(bits,21, 7);
    // Hour: bits 29-34 (6), parity 35
    *hour = u64_to_bcd(bits,29, 6);
    // Day: bits 36-41 (6)
    *day = u64_to_bcd(bits,36, 6);
    // Weekday: bits 42-44 (3)
    *weekday = u64_to_bcd(bits,42, 3);
    // Month: bits 45-49 (5)
    *month = u64_to_bcd(bits,45, 5);
    // Year: bits 50-57 (8)
   *year = u64_to_bcd(bits,50, 8);
    addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_RAW, "DCF77: (raw - no parity check)  dcf77_decode_u64 %02d.%02d.%02d %02d:%02d:00 (weekday=%d)",*day,*month,*year+2000,*hour,*minute, *weekday);    
}

static int dcf77_decode_u64(const uint64_t bits, time_t* epoch_out) {
    int minute, hour, day, weekday, month, year;
    
    raw_dcf77_decode_u64(bits, &minute, &hour, &day, &weekday, &month, &year);


    // Minute: parity 28
    int minute_parity = (bits & (1ULL <<28)) >> 28;
    // Hour: parity 35
    int hour_parity = (bits & (1ULL <<35)) >>35;
    // Date: parity 58
    int date_parity = (bits & (1ULL <<58)) >>58;

    // Parity checks  
    if (u64_to_parity(bits,21,7) != minute_parity){ 
    	addLogAdv(LOG_INFO, LOG_FEATURE_RAW, "DCF77: Parity Error minute - wanted %d got %d",u64_to_parity(bits,21,7), minute_parity);
    	return 0;
    }
    if (u64_to_parity(bits,29,6) != hour_parity){
    	addLogAdv(LOG_INFO, LOG_FEATURE_RAW, "DCF77: Parity Error hour - wanted %d got %d",u64_to_parity(bits,29,6), hour_parity);
    	return 0;
    }
    if (u64_to_parity(bits,36,22) != date_parity){
    	addLogAdv(LOG_INFO, LOG_FEATURE_RAW, "DCF77: Parity Error date - wanted %d got %d",u64_to_parity(bits,36,22), date_parity);
    	return 0;
    }

    // just to be sure: a range check
    if (minute > 59 || hour > 23 || day < 1 || day > 31 || month < 1 || month > 12 || year > 99) return 0;

/*    struct tm t;
    t.tm_sec = 0;
    t.tm_min = minute;
    t.tm_hour = hour;
    t.tm_mday = day;
    t.tm_mon = month-1;
    t.tm_year = year+100; // DCF77 gives 2-digit year; assume 2000+
    t.tm_isdst = -1;
    time_t epoch = mktime(&t);
*/
    time_t epoch = (time_t)dateToEpoch((uint16_t)year+2000, month, day, hour, minute, 0);
//    const char *wdays[] = {"Sun","Mon", "Tue", "Wed", "Thu", "Fri", "Sat","Sun"}; 
//    addLogAdv(LOG_DEBUG, LOG_FEATURE_RAW, "DCF77: dcf77_decode %s %d.%d.%d %d:%d:00",wdays[weekday],day,month,year+2000,hour,minute);
    addLogAdv(LOG_DEBUG, LOG_FEATURE_RAW, "DCF77: dcf77_decode %02d.%02d.%d %02d:%02d:00",day,month,year+2000,hour,minute);

    if (epoch_out) *epoch_out = epoch;
    return 1;
}

static uint64_t dcfbits=0;	// we are storing our bits here 
static uint64_t dcfbits_last=0;	// backup to show on main page if requested 

/*
// START for DEBUG 
static uint64_t dcfbits150=0;	// we are storing our bits here 
static uint64_t dcfbits180=0;	// we are storing our bits here 
// END for DEBUG 
*/

// new DCF77 interrupt handler
static void DCF77_ISR_Common() {
    static uint64_t actbit=1;
    
    uint32_t now = get_ms_tick();
    if (dcf77_last_edge_tick == 0) dcf77_last_edge_tick = now;		// start is no detected sync ;-)

    addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_RAW, "DCF77: DEBUG: gap to prev. interrupt  %i", now - dcf77_last_edge_tick );
   
    // handle detection of "long gap" == start of sync
    if (now - dcf77_last_edge_tick > DCF77_SYNC_THRESHOLD_MS){
//           addLogAdv(LOG_DEBUG, LOG_FEATURE_RAW, "DCF77: DEBUG: detected sync pulse > %i", now - dcf77_last_edge_tick );
           dcf77_pulse_count = 0;
           dcfbits=0;
/*
// START for DEBUG 
dcfbits150=0;
dcfbits180=0;
// END for DEBUG 
*/
           actbit=1;
           dcf77_synced = true;
           settime=0;
    }
    
    
    /*
	// We started by defining "STARTEDGE" to see wich edge "starts" the bit
	//
	// and to toggle between RISING and FALLING we define TRIGGER_START and TRIGGER_END
	// to change the edge after we found one     
    
#if PLATFORM_BEKEN
    if (STARTEDGE == RISING){
    	uint8_t TRIGGER_START = IRQ_TRIGGER_RISING_EDGE, TRIGGER_END = IRQ_TRIGGER_FALLING_EDGE; 
    }
    else {
    	uint8_t TRIGGER_START = IRQ_TRIGGER_FALLING_EDGE, TRIGGER_END = IRQ_TRIGGER_RISING_EDGE; 
    }
#elif PLATFORM_BL602
    if (STARTEDGE == RISING){
    	uint8_t TRIGGER_START = GPIO_INT_TRIG_POS_PULSE, TRIGGER_END = GPIO_INT_TRIG_NEG_PULSE; 
    }
    else {
    	uint8_t TRIGGER_START = GPIO_INT_TRIG_NEG_PULSE, TRIGGER_END = GPIO_INT_TRIG_POS_PULSE; 
    }
#else
#endif


    
    */
#if PLATFORM_BEKEN
    next_IRQ ^= 1;	// toggle IRQ between 0 and 1

    if (next_IRQ != STARTEDGE){		// caution, we already toggled the value
    	gpio_int_enable(GPIO_DCF77, TRIGGER_END , DCF77_Interrupt);
    	dcf77_last_edge_tick = now;
    	return;
    } else {
    	gpio_int_enable(GPIO_DCF77, TRIGGER_START , DCF77_Interrupt);
    }
#elif PLATFORM_BL602
    next_IRQ ^= 1;	// toggle IRQ between 0 and 1

    if (next_IRQ != STARTEDGE){		// caution, we already toggled the value
	hal_gpio_register_handler(DCF77_Interrupt, GPIO_DCF77, GPIO_INT_CONTROL_ASYNC, TRIGGER_END, (void*)NULL);
    	dcf77_last_edge_tick = now;
    	return;
    } else {
	hal_gpio_register_handler(DCF77_Interrupt, GPIO_DCF77, GPIO_INT_CONTROL_ASYNC, TRIGGER_START, (void*)NULL);
    }
#else

    // this ISR is for both rising and falling, 
    // if STARTEDGE == RISING (RISING = 0) pulse starts with rising edge  --> check, if pin is 1 after interrupt
    // if STARTEDGE == FALLING (FALLING = 1) pulse starts with falling edge  --> check, if pin is 0 after interrupt
    // --> check for !=  STARTEDGE 
    if (HAL_PIN_ReadDigitalInput(GPIO_DCF77) != STARTEDGE){		// start of "bit"
    	dcf77_last_edge_tick = now;
    	return;
    }
#endif
    // wait for sync
    if (!dcf77_synced) {
        // Not synced yet, do nothing
               return;
    }

    // if we reached this point
    //		we are synced and 
    //		we just had the edge "ending" the bit 
    uint32_t pulse_length = now - dcf77_last_edge_tick;
//    addLogAdv(LOG_DEBUG, LOG_FEATURE_RAW, "DCF77: DEBUG: actual pulse length %i (count=%i)", pulse_length, dcf77_pulse_count);
    if (dcf77_pulse_count < 59) {
        if ( dcf77_pulse_count > 20 && // ignore "lenght" errors outside of time information (starts at bit 21)
        	(pulse_length < DCF77_MIN_PULSE_MS || pulse_length > DCF77_MAX_PULSE_MS || (pulse_length > end0 && pulse_length < start1))) {
            // Invalid pulse, lose sync
            dcf77_synced = false;
            dcf77_pulse_count = 0;
            actbit=1;
            addLogAdv(LOG_INFO, LOG_FEATURE_RAW, "DCF77: bit illegal pulse length %i", pulse_length );
            return;
        }
        // pulse is of valid length. All bits are 0, so only on a long pulse we need to set this position to 1
        if (pulse_length > start1) dcfbits |= actbit;
/*
// START for DEBUG 
        if (pulse_length > 150) dcfbits150 |= actbit;
        if (pulse_length > 180) dcfbits180 |= actbit;
// END for DEBUG 
*/

//        addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_RAW, "DCF77: bit %i is %i", dcf77_pulse_count,  (pulse_length > 150));
        dcf77_pulse_count++;
        actbit <<= 1;
    }
    if (dcf77_pulse_count == 59) {
        // Buffer full, copy to decode for next minute (outside ISR)
        settime=1;
       // Wait for next sync
       dcf77_synced = false;
    }
}



#if PLATFORM_W600 || PLATFORM_W800
static void DCF77_Interrupt(void* context) {
    tls_clr_gpio_irq_status(GPIO_DCF77_pin);
    DCF77_ISR_Common();
}
#elif PLATFORM_BL602
static void DCF77_Interrupt(void* arg) {
    DCF77_ISR_Common();
    bl_gpio_intmask(GPIO_DCF77, 0);
}
#elif PLATFORM_LN882H
// handler are defined in BL0937
#if ! (ENABLE_DRIVER_BL0937)
void GPIOA_IRQHandler() {
    uint32_t base = GPIO_DCF77 < 16 ? GPIOA_BASE : GPIOB_BASE;
    uint16_t gpio_pin = (uint16_t)1 << (uint16_t)(GPIO_DCF77 % 16);
    if(hal_gpio_pin_get_it_flag(base, gpio_pin) == HAL_SET) {
        hal_gpio_pin_clr_it_flag(base, gpio_pin);
        DCF77_ISR_Common();
    }
}
void GPIOB_IRQHandler() {
    GPIOA_IRQHandler(); // Same logic for simplicity
}
#endif
#elif PLATFORM_BEKEN
static void DCF77_Interrupt(void* arg) {
    DCF77_ISR_Common();
}
#elif PLATFORM_REALTEK
void dcf77_irq_handler(uint32_t id, gpio_irq_event event) {
    DCF77_ISR_Common();
}
#elif PLATFORM_ECR6600
void DCF77_Interrupt(unsigned char pinNum) {
    DCF77_ISR_Common();
}
#elif PLATFORM_XRADIO
static void DCF77_Interrupt(void* arg) {
    DCF77_ISR_Common();
}
#elif PLATFORM_ESP8266 || PLATFORM_ESPIDF
static void DCF77_Interrupt(void* arg) {
    DCF77_ISR_Common();
}
#else
void DCF77_Interrupt(unsigned char pinNum) {
    DCF77_ISR_Common();
}
#endif

void DCF77_Init_Pin() {
    GPIO_DCF77 = PIN_FindPinIndexForRole(IOR_DCF77, -1);
    if (GPIO_DCF77 < 0 ){ 
    	addLogAdv(LOG_ERROR, LOG_FEATURE_RAW, "DCF77: No pin defined for DCF77 signal!");
    	return;
    }
    HAL_PIN_Setup_Input_Pulldown(GPIO_DCF77);

#if PLATFORM_W600 || PLATFORM_W800
    GPIO_DCF77_pin = HAL_GetGPIOPin(GPIO_DCF77);
    tls_gpio_cfg(GPIO_DCF77_pin, WM_GPIO_DIR_INPUT, WM_GPIO_ATTR_PULLLOW);
    tls_gpio_isr_register(GPIO_DCF77_pin, DCF77_Interrupt, NULL);
    tls_gpio_irq_enable(GPIO_DCF77_pin, WM_GPIO_IRQ_TRIG_DOUBLE_EDGE);
#elif PLATFORM_BL602
    hal_gpio_register_handler(DCF77_Interrupt, GPIO_DCF77, GPIO_INT_CONTROL_ASYNC, GPIO_INT_TRIG_POS_PULSE, (void*)NULL);
#elif PLATFORM_LN882H
    hal_gpio_pin_it_cfg(GPIO_DCF77 < 16 ? GPIOA_BASE : GPIOB_BASE, (uint16_t)1 << (GPIO_DCF77 % 16), GPIO_INT_FALLING);
    hal_gpio_pin_it_en(GPIO_DCF77 < 16 ? GPIOA_BASE : GPIOB_BASE, (uint16_t)1 << (GPIO_DCF77 % 16), HAL_ENABLE);
    NVIC_SetPriority(GPIO_DCF77 < 16 ? GPIOA_IRQn : GPIOB_IRQn, 1);
    NVIC_EnableIRQ(GPIO_DCF77 < 16 ? GPIOA_IRQn : GPIOB_IRQn);
#elif PLATFORM_BEKEN
    gpio_int_enable(GPIO_DCF77, IRQ_TRIGGER_RISING_EDGE, DCF77_Interrupt);
#elif PLATFORM_REALTEK
    rtl_dcf77 = g_pins + GPIO_DCF77;
    rtl_dcf77->irq = os_malloc(sizeof(gpio_irq_t));
    memset(rtl_dcf77->irq, 0, sizeof(gpio_irq_t));
    gpio_irq_init(rtl_dcf77->irq, rtl_dcf77->pin, dcf77_irq_handler, NULL);
    gpio_irq_set(rtl_dcf77->irq, IRQ_FALL, 1);
    gpio_irq_enable(rtl_dcf77->irq);
#elif PLATFORM_ECR6600
    T_GPIO_ISR_CALLBACK dcf77isr;
    dcf77isr.gpio_callback = (&DCF77_Interrupt);
    dcf77isr.gpio_data = 0;
    drv_gpio_ioctrl(GPIO_DCF77, DRV_GPIO_CTRL_INTR_MODE, DRV_GPIO_ARG_INTR_MODE_N_EDGE);
    drv_gpio_ioctrl(GPIO_DCF77, DRV_GPIO_CTRL_REGISTER_ISR, (int)&dcf77isr);
    drv_gpio_ioctrl(GPIO_DCF77, DRV_GPIO_CTRL_INTR_ENABLE, 0);
#elif PLATFORM_XRADIO
    xr_dcf77 = g_pins + GPIO_DCF77;
    HAL_XR_ConfigurePin(xr_dcf77->port, xr_dcf77->pin, GPIOx_Pn_F6_EINT, GPIO_PULL_UP);
    GPIO_IrqParam dcf77param;
    dcf77param.event = GPIO_IRQ_EVT_FALLING_EDGE;
    dcf77param.callback = DCF77_Interrupt;
    dcf77param.arg = (void*)0;
    HAL_GPIO_EnableIRQ(xr_dcf77->port, xr_dcf77->pin, &dcf77param);
#elif PLATFORM_ESP8266 || PLATFORM_ESPIDF
    esp_dcf77 = g_pins + GPIO_DCF77;
    gpio_install_isr_service(0);
    ESP_ConfigurePin(esp_dcf77->pin, GPIO_MODE_INPUT, true, false, GPIO_INTR_ANYEDGE);
    gpio_isr_handler_add(esp_dcf77->pin, DCF77_Interrupt, NULL);
#else
 
#endif
}

void DCF77_Shutdown_Pin() {
#if PLATFORM_W600 || PLATFORM_W800
    tls_gpio_irq_disable(GPIO_DCF77_pin);
#elif PLATFORM_BEKEN
    gpio_int_disable(GPIO_DCF77);
#elif PLATFORM_REALTEK
    gpio_irq_free(rtl_dcf77->irq);
    os_free(rtl_dcf77->irq);
    rtl_dcf77->irq = NULL;
#elif PLATFORM_ECR6600
    drv_gpio_ioctrl(GPIO_DCF77, DRV_GPIO_CTRL_INTR_DISABLE, 0);
#elif PLATFORM_XRADIO
    HAL_GPIO_DeInit(xr_dcf77->port, xr_dcf77->pin);
    HAL_GPIO_DisableIRQ(xr_dcf77->port, xr_dcf77->pin);
#elif PLATFORM_ESP8266 || PLATFORM_ESPIDF
    gpio_isr_handler_remove(esp_dcf77->pin);
    gpio_uninstall_isr_service();
#endif
}

static time_t act_epoch;
static time_t last_epoch;

void DCF77_OnEverySecond() {

    if (GPIO_DCF77 < 0){
	addLogAdv(LOG_DEBUG, LOG_FEATURE_RAW, "DCF77: No pin defined! \n");
    	return;
    }

    if (settime != 0){
    	if (settime == 1){
		uint8_t ST = (uint8_t)((dcfbits >> 17) & 3);	// bits 17 and 18 are Z1 and Z2   - 01 = CEST / 10 = CET  (everything else: illegal) 
/*
// START for DEBUG 
    		dcf77_decode_u64(dcfbits150, &act_epoch) ? 2 : 3;
    		dcf77_decode_u64(dcfbits180, &act_epoch) ? 2 : 3;
// END for DEBUG 
*/
    		settime = dcf77_decode_u64(dcfbits, &act_epoch) ? 2 : 3;
    		dcfbits_last = dcfbits;
    		addLogAdv(LOG_DEBUG, LOG_FEATURE_RAW, "DCF77: Summer time bits Z1 and Z2: %d (%s)\n",ST, ST==1? "summer time": ST==2 ? "winter time" : "illegal");
    		// DCF77 will allways broadcast local time in Germany. So to get UTC: if ST==1 (summer time) sub 2h, if ST==2 (winter time) sub 1h
    		act_epoch -=  3600 * (3-ST);
//    		addLogAdv(LOG_DEBUG, LOG_FEATURE_RAW, "DCF77: UTC calculated: %llu (previous: %llu - diff=%llu seconds)\n",act_epoch, last_epoch, (act_epoch - last_epoch));
		if (ST < 1 || ST > 2){
			addLogAdv(LOG_ERROR, LOG_FEATURE_RAW, "DCF77: Illegal state of DST indcator (%d)! Not setting time \n",ST);
			settime=4;
		}
    	} else {
    		// do this in "else" path, so it's 1 second after we got all bits ( = second 59)
    		
    		if (settime==2){
    			// as an additonal sanity check: last time decoded must be one exactly minute (60 seconds) before! 
    			addLogAdv(LOG_DEBUG, LOG_FEATURE_RAW, "DCF77: UTC calculated: %u (previous: %u - diff=%u seconds)\n",(uint32_t)act_epoch, (uint32_t)last_epoch, (uint32_t)(act_epoch - last_epoch));
//    			if ( (act_epoch - 60) == last_epoch ) TIME_setDeviceTime((uint32_t)act_epoch);
    			TIME_setDeviceTime((uint32_t)act_epoch);
    			last_epoch = act_epoch;
    			lastset = g_secondsElapsed;
    		}
    	}
    }


    char str[60]={0};
    int i;

    // Iterate through each bit from the least significant to the most significant
    for (i = 0; i < dcf77_pulse_count && i < 59; i++) {
        // Print the bit at position i
        str[i]=((dcfbits >> i) & 1)?'1':'0';
    }
    addLogAdv(LOG_DEBUG, LOG_FEATURE_RAW, "DCF77: %ssynced - state= %i - got %02i bits: %s \n",dcf77_synced?"":"not ",settime, i,str);
    	    	
}


void DCF77_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState)
{
	if (bPreState || ! httpstat){
		return;
	}
	int minute, hour, day, weekday, month, year;
	raw_dcf77_decode_u64(dcfbits_last, &minute, &hour, &day, &weekday, &month, &year);
	
//	    addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_RAW, "DCF77: (raw - no parity check)  dcf77_decode_u64 %02d.%02d.%02d %02d:%02d:00 (weekday=%d)",day,month,year+2000,hour,minute, weekday);    

	
	hprintf255(request, "<h5>DCF77: last raw info:  %02d.%02d.%02d %02d:%02d:00 (weekday=%d) - last time set %i secs ago</h5>",day,month,year+2000,hour,minute, weekday, g_secondsElapsed - lastset);
}

void DCF77_Init(void) {
	// look for arguments
	uint8_t temp=Tokenizer_GetArgsCount()-1;
	const char* arg;
	const char* fake=NULL;	
	for (int i=1; i<=temp; i++) {
		arg = Tokenizer_GetArg(i);
		
		addLogAdv(LOG_DEBUG, LOG_FEATURE_RAW,"DCF77: argument %i/%i is %s",i,temp,arg);		

		if ( arg && !stricmp(arg,"fall")) {
			 STARTEDGE = FALLING;

		} 
		if ( arg && !stricmp(arg,"httpstat")) {
			 httpstat = true;

		} 
		fake=strstr(arg, "end0=");
		if ( arg && fake ) {
			int i=0;
			fake += 5;
			end0 = atoi(fake);
			addLogAdv(LOG_DEBUG, LOG_FEATURE_RAW,"DCF77: set max duration of 0 bit to %ims",end0);		
		} 
		fake=NULL;
		fake=strstr(arg, "start1=");
		if ( arg && fake ) {
			int i=0;
			fake += 7;
			start1 = atoi(fake);
			addLogAdv(LOG_DEBUG, LOG_FEATURE_RAW,"DCF77: set min duration of 1 bit to %ims",start1);		
		} 
		if (Tokenizer_IsArgInteger(i)){
			uint8_t XXX = Tokenizer_GetArgInteger(i);
			addLogAdv(LOG_DEBUG, LOG_FEATURE_RAW,"DCF77: set XXX to %i",XXX);
		}
	}			


    addLogAdv(LOG_INFO, LOG_FEATURE_RAW, "DCF77: DCF77_Init_Pin()\n");
    DCF77_Init_Pin();
    dcf77_pulse_count = 0;
    dcf77_synced = false;
    next_IRQ = STARTEDGE;
#if PLATFORM_BEKEN
    if (STARTEDGE == RISING){
    	TRIGGER_START = IRQ_TRIGGER_RISING_EDGE;
    	TRIGGER_END = IRQ_TRIGGER_FALLING_EDGE; 
    }
    else {
    	TRIGGER_START = IRQ_TRIGGER_FALLING_EDGE;
    	TRIGGER_END = IRQ_TRIGGER_RISING_EDGE; 
    }
#elif PLATFORM_BL602
    if (STARTEDGE == RISING){
    	TRIGGER_START = GPIO_INT_TRIG_POS_PULSE;
    	TRIGGER_END = GPIO_INT_TRIG_NEG_PULSE; 
    }
    else {
    	TRIGGER_START = GPIO_INT_TRIG_NEG_PULSE;
    	TRIGGER_END = GPIO_INT_TRIG_POS_PULSE; 
    }
#else
#endif

}
void DCF77_Stop(){
    DCF77_Shutdown_Pin();
};
#endif // ENABLE_DRIVER_DCF77
