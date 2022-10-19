
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
    //#include "pwm.h"
    #include "pwm_pub.h"

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
static UINT32 ir_div = 1;
static UINT32 ir_periodus = 50;

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

#include "../libraries/Arduino-IRremote-mod/src/IRProtocol.h"

// this is to replicate places where the library uses the static class.
// will need to update to call our dynamic class
class SpoofIrSender {
    public:
        void enableIROut(uint_fast8_t freq){

        }
        void mark(unsigned int  aMarkMicros){

        }
        void space(unsigned int  aMarkMicros){

        }
        void sendPulseDistanceWidthFromArray(uint_fast8_t aFrequencyKHz, unsigned int aHeaderMarkMicros,
            unsigned int aHeaderSpaceMicros, unsigned int aOneMarkMicros, unsigned int aOneSpaceMicros, unsigned int aZeroMarkMicros,
            unsigned int aZeroSpaceMicros, uint32_t *aDecodedRawDataArray, unsigned int aNumberOfBits, bool aMSBFirst,
            bool aSendStopBit, unsigned int aRepeatPeriodMillis, int_fast8_t aNumberOfRepeats) {

        }
        void sendPulseDistanceWidthFromArray(PulsePauseWidthProtocolConstants *aProtocolConstants, uint32_t *aDecodedRawDataArray,
            unsigned int aNumberOfBits, int_fast8_t aNumberOfRepeats) {
            
        }

};

SpoofIrSender IrSender;

// this is the actual IR library include.
// it's all in .h and .hpp files, no .c or .cpp
#include "../libraries/Arduino-IRremote-mod/src/IRremote.hpp"

static int PIN_GetPWMIndexForPinIndex(int pin) {
	if(pin == 6)
		return 0;
	if(pin == 7)
		return 1;
	if(pin == 8)
		return 2;
	if(pin == 9)
		return 3;
	if(pin == 24)
		return 4;
	if(pin == 26)
		return 5;
	return -1;
}

// override aspects of sending for our own interrupt driven sends
// basically, IRsend calls mark(us) and space(us) to send.
// we simply note the numbers into a rolling buffer, assume the first is a mark()
// and then every 50us service the rolling buffer, changing the PWM from 0 duty to 50% duty
// appropriately.
#define SEND_MAXBITS 128
class myIRsend : public IRsend {
    public:
        myIRsend(uint_fast8_t aSendPin){
            //IRsend::IRsend(aSendPin); - has been called already?
            our_us = 0;
            our_ms = 0;
            resetsendqueue();
        }

        void enableIROut(uint_fast8_t aFrequencyKHz){
            // just setup variables for use in ISR
            pwmfrequency = ((uint32_t)aFrequencyKHz) * 1000;
        	pwmperiod = (26000000 / pwmfrequency);
            pwmduty = pwmperiod/2;
        }

        uint32_t millis(){
            return our_ms;
        }
        void delay(long int ms){
            // add a pure delay to our queue
        	ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"Delay %dms", ms);
            space(ms*1000);
        }


        using IRsend::write;

        void mark(unsigned int aMarkMicros){
            // sends a high for aMarkMicros
            uint32_t newtimein = (timein + 1)%(SEND_MAXBITS * 2);
            if (newtimein != timeout){
                // store mark bits in highest +ve bit of count
                times[timein] = aMarkMicros | 0x10000000;
                timein = newtimein;
                timecount++;
                timecounttotal++;
            } else {
                overflows++;
            }
        }
        void space(unsigned int aMarkMicros){
            // sends a low for aMarkMicros
            uint32_t newtimein = (timein + 1)%(SEND_MAXBITS * 2);
            if (newtimein != timeout){
                times[timein] = aMarkMicros;
                timein = newtimein;
                timecount++;
                timecounttotal++;
            } else {
                overflows++;
            }
        }

        void resetsendqueue(){
            // sends a low for aMarkMicros
            timein = timeout = 0;
            timecount = 0;
            overflows = 0;
            currentsendtime = 0;
            currentbitval = 0;
            timecounttotal = 0;
        }
        int32_t times[SEND_MAXBITS * 2]; // enough for 128 bits
        unsigned short timein;
        unsigned short timeout;
        unsigned short timecount;
        unsigned short overflows;
        uint32_t timecounttotal;

        int32_t getsendqueue(){
            int32_t val = 0;
            if (timein != timeout){
                val = times[timeout];
                timeout = (timeout + 1)%(SEND_MAXBITS * 2);
                timecount--;
            }
            return val;
        }

        int currentsendtime;
        int currentbitval;

        uint8_t sendPin;
        uint8_t pwmIndex;
        uint32_t pwmfrequency;
        uint32_t pwmperiod;
        uint32_t pwmduty;

        uint32_t our_ms;
        uint32_t our_us;
};


// our send/receive instances
myIRsend *pIRsend = NULL;
IRrecv *ourReceiver = NULL;

// this is our ISR.
// it is called every 50us, so we need to work on making it as efficient as possible.
extern "C" void DRV_IR_ISR(UINT8 t){
    if (pIRsend && (pIRsend->pwmIndex >= 0)){
        pIRsend->our_us += 50;
        if (pIRsend->our_us > 1000){
            pIRsend->our_ms++;
            pIRsend->our_us -= 1000;
        }

        int pinval = 0;
        if (pIRsend->currentsendtime){
            pIRsend->currentsendtime -= ir_periodus;
            if (pIRsend->currentsendtime <= 0){
                int32_t remains = pIRsend->currentsendtime;
                int32_t newtime = pIRsend->getsendqueue();
                if (0 == newtime){
                    // if it was the last one
                    pIRsend->currentsendtime = 0;    
                    pIRsend->currentbitval = 0;
                } else {
                    // we got a new time
                    // store mark bits in highest +ve bit of count
                    pIRsend->currentbitval = (newtime & 0x10000000)? 1:0;
                    pIRsend->currentsendtime = (newtime & 0xfffffff);
                    // adjust the us value to keep the running accuracy
                    // and avoid a running error?
                    // note remains is -ve
                    pIRsend->currentsendtime += remains;
                }
            }
        } else {
            int32_t newtime = pIRsend->getsendqueue();
            if (!newtime){
                pIRsend->currentsendtime = 0;
                pIRsend->currentbitval = 0;
            } else {
                pIRsend->currentsendtime = (newtime & 0xfffffff);
                pIRsend->currentbitval = (newtime & 0x10000000)? 1:0;
            }
        }
        pinval = pIRsend->currentbitval;

        uint32_t duty = pIRsend->pwmduty;
        if (!pinval){
            duty = 0;
        }
#if PLATFORM_BK7231N
        bk_pwm_update_param((bk_pwm_t)pIRsend->pwmIndex, pIRsend->pwmperiod, duty,0,0);
#else
        bk_pwm_update_param((bk_pwm_t)pIRsend->pwmIndex, pIRsend->pwmperiod, duty);
#endif
    }

    IR_ISR();
    ir_counter++;
}



// test routine to start IR RX and TX
// currently fixed pins for testing.
extern "C" void DRV_IR_Init(){
	ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"Log from extern C CPP");

    unsigned char pin = 9;// PWM3/25
    unsigned char txpin = 24;// PWM3/25

	// allow user to change them
	pin = PIN_FindPinIndexForRole(IOR_IRRecv,pin);
	txpin = PIN_FindPinIndexForRole(IOR_IRSend,txpin);

    if (ourReceiver){
        IRrecv *temp = ourReceiver;
        ourReceiver = NULL;
        delete temp;
    }

    ourReceiver = new IRrecv(pin);
    ourReceiver->start();

    if (pIRsend){
        myIRsend *pIRsendTemp = pIRsend;
        pIRsend = NULL;
        delete pIRsendTemp;
    }

	int pwmIndex = PIN_GetPWMIndexForPinIndex(txpin);
	// is this pin capable of PWM?
	if(pwmIndex != -1) {
        uint32_t pwmfrequency = 38000;
    	uint32_t period = (26000000 / pwmfrequency);
        uint32_t duty = period/2;
#if PLATFORM_BK7231N
	    // OSStatus bk_pwm_initialize(bk_pwm_t pwm, uint32_t frequency, uint32_t duty_cycle);
	    bk_pwm_initialize((bk_pwm_t)pwmIndex, period, duty, 0, 0);
#else
	    bk_pwm_initialize((bk_pwm_t)pwmIndex, period, duty);
#endif
        bk_pwm_start((bk_pwm_t)pwmIndex);
        myIRsend *pIRsendTemp = new myIRsend((uint_fast8_t) txpin);
        pIRsendTemp->resetsendqueue();
        pIRsendTemp->pwmIndex = pwmIndex;
        pIRsendTemp->pwmfrequency = pwmfrequency;
        pIRsendTemp->pwmperiod = period;
        pIRsendTemp->pwmduty = duty;

        pIRsend = pIRsendTemp;
        //bk_pwm_stop((bk_pwm_t)pIRsend->pwmIndex);
	}
}


// log the received IR
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
// currently called once per sec from user_main timer
// should probably be called every 100ms.
extern "C" void DRV_IR_RunFrame(){
    if (ir_counter){
        //ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"IR counter: %u", ir_counter);
    }
    if (pIRsend){
        if (pIRsend->overflows){
            ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"##### IR send overflows %d", (int)pIRsend->overflows);
            pIRsend->resetsendqueue();
        } else {
            ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"IR send count %d remains %d currentus %d", (int)pIRsend->timecounttotal, (int)pIRsend->timecount, (int)pIRsend->currentsendtime);
        }
    }

    if (ourReceiver){
        if (ourReceiver->decode()) {
            PrintIRData(&ourReceiver->decodedIRData);
            ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"IR decode returned true, protocol %d", (int)ourReceiver->decodedIRData.protocol);

            if (pIRsend){
                pIRsend->write(&ourReceiver->decodedIRData, (int_fast8_t) 2);

                ADDLOG_INFO(LOG_FEATURE_CMD, (char *)"IR send timein %d timeout %d", (int)pIRsend->timein, (int)pIRsend->timeout);
            }

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





#ifdef TEST_CPP
// routines to test C++
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



#endif // platform T