
#if PLATFORM_BK7231T

extern "C" {
    // these cause error: conflicting declaration of 'int bk_wlan_mcu_suppress_and_sleep(unsigned int)' with 'C' linkage
    #include "include.h"
    #include "arm_arch.h"
    #include "../new_pins.h"
    #include "../new_cfg.h"
    #include "../logging/logging.h"
    #include "../obk_config.h"
    #include "bk_timer_pub.h"
    #include "drv_model_pub.h"
    #include <gpio_pub.h>

    #include "../../beken378/func/include/net_param_pub.h"
    #include "../../beken378/func/user_driver/BkDriverPwm.h"
    #include "../../beken378/func/user_driver/BkDriverI2c.h"
    #include "../../beken378/driver/i2c/i2c1.h"
    #include "../../beken378/driver/gpio/gpio.h"

    #include <ctype.h>

    unsigned long ir_counter = 0;

}


//#define USE_IRREMOTE_HPP_AS_PLAIN_INCLUDE 1
#undef read
#undef write
#define PROGMEM


#define NO_LED_FEEDBACK_CODE 1

//typedef unsigned char uint_fast8_t;
typedef unsigned short uint16_t;

#define __FlashStringHelper char

// dummy functions
void noInterrupts(){}
void interrupts(){}
unsigned long millis(){ 
    return 0; 
}
unsigned long micros(){ 
    return 0;
}

void delay(int n){
    return;
}

void delayMicroseconds(int n){
    return;
}

class Print {
    public:
        void println(const char *p){
            return;
        }
        void print(...){
            return;
        }
};

Print Serial;


#define INPUT 0
#define OUTPUT 1
#define HIGH 1
#define LOW 1


void digitalToggleFast(unsigned char P) {
    bk_gpio_output((GPIO_INDEX)P, !bk_gpio_input((GPIO_INDEX)P));
}

unsigned char digitalReadFast(unsigned char P) { 
	return bk_gpio_input((GPIO_INDEX)P);
}

void digitalWriteFast(unsigned char P, unsigned char V) {
    //RAW_SetPinValue(P, V);
    //HAL_PIN_SetOutputValue(index, iVal);
    bk_gpio_output((GPIO_INDEX)P, V);
}

void pinModeFast(unsigned char P, unsigned char V) {
    if (V == INPUT){
        bk_gpio_config_input_pup((GPIO_INDEX)P);
    }
}


#define EXTERNAL_IR_TIMER_ISR

//////////////////////////////////////////
// our external timer interrupt stuff
// this will have already been done
#define TIMER_RESET_INTR_PENDING


#  if defined(ISR)
#undef ISR
#  endif
#define ISR void IR_ISR
extern "C" void DRV_IR_ISR(UINT8 t);

static UINT32 ir_chan = BKTIMER0;
static UINT32 ir_periodus = 50;
static UINT32 ir_div = 1;

void timerConfigForReceive() {
    ir_counter = 0;

    timer_param_t params = {
        (unsigned char) ir_chan,
        (unsigned char) ir_div, // div
        ir_periodus, // us
        DRV_IR_ISR
    };
    //GLOBAL_INT_DECLARATION();

	ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"ir timer init");
    bk_timer_init();
	ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"ir timer init done");
    UINT32 res;
    res = sddev_control((char *)TIMER_DEV_NAME, CMD_TIMER_INIT_PARAM_US, &params);
	ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"ir timer setup %u", res);
    res = sddev_control((char *)TIMER_DEV_NAME, CMD_TIMER_UNIT_ENABLE, &ir_chan);
	ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"ir timer enabled %u", res);
}

static void timer_enable(){
    UINT32 res;
    res = sddev_control((char *)TIMER_DEV_NAME, CMD_TIMER_UNIT_ENABLE, &ir_chan);
	ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"ir timer enabled %u", res);
}
static void timer_disable(){
    UINT32 res;
    res = sddev_control((char *)TIMER_DEV_NAME, CMD_TIMER_UNIT_DISABLE, &ir_chan);
	ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"ir timer disabled %u", res);
}

#define TIMER_ENABLE_RECEIVE_INTR timer_enable();
#define TIMER_DISABLE_RECEIVE_INTR timer_disable();

//////////////////////////////////////////

class SpoofIrReceiver {
    public:
        static void restartAfterSend(){

        }
};

SpoofIrReceiver IrReceiver;

#include "../libraries/Arduino-IRremote-mod/src/IRRemote.hpp"

int pincounters[32] = {0};
UINT32 pinvals[32] = {0};

// this is our ISR
extern "C" void DRV_IR_ISR(UINT8 t){
    IR_ISR();
    ir_counter++;

#ifdef READ_ALL_PINS
    UINT32 pins = 0;
    for (int id = 0; id < 32; id++){
        UINT32 val = 0;
        volatile UINT32 *gpio_cfg_addr;
        gpio_cfg_addr = (volatile UINT32 *)(REG_GPIO_CFG_BASE_ADDR + id * 4);
        val = REG_READ(gpio_cfg_addr);

        //val = val & GCFG_INPUT_BIT);
        UINT32 pinval = (val & GCFG_INPUT_BIT);
        if (pinvals[id] != pinval){
            pincounters[id]++;
            pinvals[id] = pinval;
        }
        if (val & GCFG_INPUT_BIT){
            pins |= 1<<id;
        }
    }
#endif

}


IRrecv* ourReceiver = NULL;

#ifdef TEST_CPP
class cpptest2 {
    public:
        int initialised;
        cpptest2(){
        	// remove else static class may kill us!!!ADDLOG_INFO(LOG_FEATURE_CMD, "Log from Class constructor");
            initialised = 42;
        };
        ~cpptest2(){
            initialised = 24;
        	ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"Log from Class destructor");
        }

        void print(){
        	ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"Log from Class %d", initialised);
        }
};

cpptest2 staticclass;

void cpptest(){
	ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"Log from CPP");
    cpptest2 test;
    test.print();
    cpptest2 *test2 = new cpptest2();
    test2->print();
	ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"Log from static class (is it initialised?):");
    staticclass.print();
}
#endif


extern "C" void testmehere(){
	ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"Log from extern C CPP");

    unsigned char pin = 9;// PWM3/25

    ourReceiver = new IRrecv(pin);

    ourReceiver->start();
}


void PrintIRData(IRData *aIRDataPtr){
    ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"IR decode returned true, protocol %d", (int)aIRDataPtr->protocol);
    if (aIRDataPtr->protocol == UNKNOWN) {
#if defined(DECODE_HASH)
        ADDLOG_INFO(LOG_FEATURE_CMD, (char *)" Hash=0x%X", (int)aIRDataPtr->decodedRawData);
#endif
        ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"%d bits (incl. gap and start) received", (int)((aIRDataPtr->rawDataPtr->rawlen + 1) / 2));
    } else {
#if defined(DECODE_DISTANCE)
        if(aIRDataPtr->protocol != PULSE_DISTANCE) {
#endif
        /*
         * New decoders have address and command
         */
        ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"Address=0x%X", (int)aIRDataPtr->address);
        ADDLOG_INFO(LOG_FEATURE_CMD, (char *)" Command=0x%X", (int)aIRDataPtr->command);

        if (aIRDataPtr->flags & IRDATA_FLAGS_EXTRA_INFO) {
            ADDLOG_INFO(LOG_FEATURE_CMD, (char *)" Extra=0x%X", (int)aIRDataPtr->extra);
        }

        if (aIRDataPtr->flags & IRDATA_FLAGS_PARITY_FAILED) {
            ADDLOG_INFO(LOG_FEATURE_CMD, (char *)" Parity fail");
        }

        if (aIRDataPtr->flags & IRDATA_TOGGLE_BIT_MASK) {
            if (aIRDataPtr->protocol == NEC) {
                ADDLOG_INFO(LOG_FEATURE_CMD, (char *)" Special repeat");
            } else {
                ADDLOG_INFO(LOG_FEATURE_CMD, (char *)" Toggle=1");
            }
        }
#if defined(DECODE_DISTANCE)
        }
#endif
        if (aIRDataPtr->flags & (IRDATA_FLAGS_IS_AUTO_REPEAT | IRDATA_FLAGS_IS_REPEAT)) {
            if (aIRDataPtr->flags & IRDATA_FLAGS_IS_AUTO_REPEAT) {
                ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"Auto-Repeat");
            } else {
                ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"Repeat");
            }
            if (1) {
                ADDLOG_INFO(LOG_FEATURE_CMD, (char *)" Gap %uus", (uint32_t)aIRDataPtr->rawDataPtr->rawbuf[0] * MICROS_PER_TICK);
            }
        }

        /*
         * Print raw data
         */
        if (!(aIRDataPtr->flags & IRDATA_FLAGS_IS_REPEAT) || aIRDataPtr->decodedRawData != 0) {
            ADDLOG_INFO(LOG_FEATURE_CMD, (char *)" Raw-Data=0x%X", aIRDataPtr->decodedRawData);

            /*
             * Print number of bits processed
             */
            ADDLOG_INFO(LOG_FEATURE_CMD, (char *)" %d bits", aIRDataPtr->numberOfBits);

            if (aIRDataPtr->flags & IRDATA_FLAGS_IS_MSB_FIRST) {
                ADDLOG_INFO(LOG_FEATURE_CMD, (char *)" MSB first", aIRDataPtr->numberOfBits);
            } else {
                ADDLOG_INFO(LOG_FEATURE_CMD, (char *)" LSB first", aIRDataPtr->numberOfBits);
            }

        } else {
            //aSerial->println();
        }

        //checkForRecordGapsMicros(aSerial, aIRDataPtr);
    }
}


////////////////////////////////////////////////////
// this polls the IR receive to see off there was any IR received
// currently called once per sec from 
extern "C" void DRV_IR_Print(){
    if (ir_counter){
        //ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"IR counter: %u", ir_counter);
    }

#ifdef READ_ALL_PINS
    UINT32 pins = 0;
    char pincounts[32*(8+1)+1];
    int id;
    for (id = 0; id < 32; id++){
        UINT32 val = 0;
        volatile UINT32 *gpio_cfg_addr;
        gpio_cfg_addr = (volatile UINT32 *)(REG_GPIO_CFG_BASE_ADDR + id * 4);
        val = REG_READ(gpio_cfg_addr);

        //val = val & GCFG_INPUT_BIT);
        if (val & GCFG_INPUT_BIT){
            pins |= 1<<id;
        }
        sprintf(&pincounts[id*(8+1)], "%08X", pincounters[id]);
        pincounts[id*(8+1)+8] = ' ';
    }
    pincounts[id*(8+1)] = 0;
    ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"GPIO pins: %08X", pins);
    ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"pincounts: %s", pincounts);
#endif

    if (ourReceiver){
        if (ourReceiver->decode()) {
            PrintIRData(&ourReceiver->decodedIRData);
            ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"IR decode returned true, protocol %d", (int)ourReceiver->decodedIRData.protocol);

            // Print a short summary of received data
            //IrReceiver.printIRResultShort(&Serial);
            //IrReceiver.printIRSendUsage(&Serial);
            if (ourReceiver->decodedIRData.protocol == UNKNOWN) {
                ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"Received noise or an unknown (or not yet enabled) protocol");
                //Serial.println(F("Received noise or an unknown (or not yet enabled) protocol"));
                // We have an unknown protocol here, print more info
                //IrReceiver.printIRResultRawFormatted(&Serial, true);
            } else {
                ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"Received cmd %08X", ourReceiver->decodedIRData.command);
            }
            //Serial.println();

            /*
            * !!!Important!!! Enable receiving of the next value,
            * since receiving has stopped after the end of the current received data packet.
            */
            ourReceiver->resume(); // Enable receiving of the next value

            /*
            * Finally, check the received data and perform actions according to the received command
            */
            if (ourReceiver->decodedIRData.command == 0x10) {
                // do something
            } else if (ourReceiver->decodedIRData.command == 0x11) {
                // do something else
            }
        }
    }
}

#endif // platform T