
#if PLATFORM_BEKEN

extern "C" {
    // these cause error: conflicting declaration of 'int bk_wlan_mcu_suppress_and_sleep(unsigned int)' with 'C' linkage
    #include "../new_common.h"

    #include "include.h"
    #include "arm_arch.h"
    #include "../new_pins.h"
    #include "../new_cfg.h"
    #include "../logging/logging.h"
    #include "../obk_config.h"
    #include "../cmnds/cmd_public.h"
    #include "bk_timer_pub.h"
    #include "drv_model_pub.h"

    // why can;t I call this?
    #include "../mqtt/new_mqtt.h"

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
    uint8_t gEnableIRSendWhilstReceive = 0;
    uint32_t gIRProtocolEnable = 0xFFFFFFFF;
    // 0 == active low.  1 = active hi
    uint8_t gIRPinPolarity = 0;

    extern int my_strnicmp(const char* a, const char* b, int len);
}

#include "drv_ir.h"

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

void timerConfigForReceive(){
    // nothing here`
}

void _timerConfigForReceive() {
    ir_counter = 0;

    timer_param_t params = {
        (unsigned char) ir_chan,
        (unsigned char) ir_div, // div
        ir_periodus, // us
        DRV_IR_ISR
    };
    //GLOBAL_INT_DECLARATION();


    UINT32 res;
    // test what error we get with an invalid command
    res = sddev_control((char *)TIMER_DEV_NAME, -1, nullptr);

    if (res == 1){
    	ADDLOG_INFO(LOG_FEATURE_IR, (char *)"bk_timer already initialised");
    } else {
    	ADDLOG_ERROR(LOG_FEATURE_IR, (char *)"bk_timer driver not initialised?");
        if ((int)res == -5){
            ADDLOG_INFO(LOG_FEATURE_IR, (char *)"bk_timer sddev not found - not initialised?");
            return;
        }
        return;
    }


	//ADDLOG_INFO(LOG_FEATURE_IR, (char *)"ir timer init");
    // do not need to do this
    //bk_timer_init();
	//ADDLOG_INFO(LOG_FEATURE_IR, (char *)"ir timer init done");
	ADDLOG_INFO(LOG_FEATURE_IR, (char *)"will ir timer setup %u", res);
    res = sddev_control((char *)TIMER_DEV_NAME, CMD_TIMER_INIT_PARAM_US, &params);
	ADDLOG_INFO(LOG_FEATURE_IR, (char *)"ir timer setup %u", res);
    res = sddev_control((char *)TIMER_DEV_NAME, CMD_TIMER_UNIT_ENABLE, &ir_chan);
	ADDLOG_INFO(LOG_FEATURE_IR, (char *)"ir timer enabled %u", res);
}

static void timer_enable(){
}
static void timer_disable(){
}
static void _timer_enable(){
    UINT32 res;
    res = sddev_control((char *)TIMER_DEV_NAME, CMD_TIMER_UNIT_ENABLE, &ir_chan);
	ADDLOG_INFO(LOG_FEATURE_IR, (char *)"ir timer enabled %u", res);
}
static void _timer_disable(){
    UINT32 res;
    res = sddev_control((char *)TIMER_DEV_NAME, CMD_TIMER_UNIT_DISABLE, &ir_chan);
	ADDLOG_INFO(LOG_FEATURE_IR, (char *)"ir timer disabled %u", res);
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





SpoofIrSender IrSender;

// this is the actual IR library include.
// it's all in .h and .hpp files, no .c or .cpp
#include "../libraries/Arduino-IRremote-mod/src/IRremote.hpp"

extern "C" int PIN_GetPWMIndexForPinIndex(int pin) ;

// override aspects of sending for our own interrupt driven sends
// basically, IRsend calls mark(us) and space(us) to send.
// we simply note the numbers into a rolling buffer, assume the first is a mark()
// and then every 50us service the rolling buffer, changing the PWM from 0 duty to 50% duty
// appropriately.

class myIRsend : public IRsend {
    public:
        myIRsend(uint_fast8_t aSendPin){
            //IRsend::IRsend(aSendPin); - has been called already?
            our_us = 0;
            our_ms = 0;
            resetsendqueue();
        }
        ~myIRsend() { }

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
        	ADDLOG_INFO(LOG_FEATURE_IR, (char *)"Delay %dms", ms);
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


// this is to replicate places where the library uses the static class.
// will need to update to call our dynamic class
void SpoofIrSender::enableIROut(uint_fast8_t freq){
	if ( Parent != NULL){

	}
}
void SpoofIrSender::mark(unsigned int  aMarkMicros){
	if ( Parent != NULL){

	}
}
void SpoofIrSender::space(unsigned int  aMarkMicros){
	if ( Parent != NULL){

	}
}
void SpoofIrSender::sendPulseDistanceWidthFromArray(uint_fast8_t aFrequencyKHz, unsigned int aHeaderMarkMicros,
    unsigned int aHeaderSpaceMicros, unsigned int aOneMarkMicros, unsigned int aOneSpaceMicros, unsigned int aZeroMarkMicros,
    unsigned int aZeroSpaceMicros, uint32_t *aDecodedRawDataArray, unsigned int aNumberOfBits, bool aMSBFirst,
    bool aSendStopBit, unsigned int aRepeatPeriodMillis, int_fast8_t aNumberOfRepeats) {
	if ( Parent != NULL){
		Parent->sendPulseDistanceWidthFromArray(aFrequencyKHz, aHeaderMarkMicros, aHeaderSpaceMicros, aOneMarkMicros, aOneSpaceMicros, aZeroMarkMicros,  aZeroSpaceMicros, aDecodedRawDataArray, aNumberOfBits,  aMSBFirst,     aSendStopBit, aRepeatPeriodMillis, aNumberOfRepeats);
	}
}
void SpoofIrSender::sendPulseDistanceWidthFromArray(PulsePauseWidthProtocolConstants *aProtocolConstants, uint32_t *aDecodedRawDataArray,
    unsigned int aNumberOfBits, int_fast8_t aNumberOfRepeats) {
	if ( Parent != NULL){
		Parent->sendPulseDistanceWidthFromArray(aProtocolConstants,aDecodedRawDataArray,aNumberOfBits,aNumberOfRepeats);
	}
}
void SpoofIrSender::SetParent(myIRsend *IRParent)
{
	this->Parent=IRParent;
}

// our send/receive instances
myIRsend *pIRsend = NULL;
IRrecv *ourReceiver = NULL;

// this is our ISR.
// it is called every 50us, so we need to work on making it as efficient as possible.
extern "C" void DRV_IR_ISR(UINT8 t){
    int sending = 0;
    if (pIRsend && (pIRsend->pwmIndex >= 0)){
        pIRsend->our_us += 50;
        if (pIRsend->our_us > 1000){
            pIRsend->our_ms++;
            pIRsend->our_us -= 1000;
        }

        int pinval = 0;
        if (pIRsend->currentsendtime){
            sending = 1;
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
                sending = 1;
                pIRsend->currentsendtime = (newtime & 0xfffffff);
                pIRsend->currentbitval = (newtime & 0x10000000)? 1:0;
            }
        }
        pinval = pIRsend->currentbitval;

        uint32_t duty = pIRsend->pwmduty;
        if (!pinval){
            if (gIRPinPolarity){
                duty = pIRsend->pwmperiod;
            } else {
                duty = 0;
            }
        }
#if PLATFORM_BK7231N
        bk_pwm_update_param((bk_pwm_t)pIRsend->pwmIndex, pIRsend->pwmperiod, duty,0,0);
#else
        bk_pwm_update_param((bk_pwm_t)pIRsend->pwmIndex, pIRsend->pwmperiod, duty);
#endif
    }

    // is someone really wants rx and TX at the same time, then allow it.
    if (gEnableIRSendWhilstReceive){
        sending = 0;
    }

    // don't receive if we are currently sending
    if (ourReceiver && !sending){
        IR_ISR();
    }
    ir_counter++;
}

extern "C" commandResult_t IR_Send_Cmd(const void *context, const char *cmd, const char *args_in, int cmdFlags) {
    int numProtocols = sizeof(ProtocolNames)/sizeof(*ProtocolNames);
    if (!args_in) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    char args[128];
    strncpy(args, args_in, sizeof(args)-1);
    args[sizeof(args)-1] = 0;

    // split arg at hyphen;
    char *p = args;
    while (*p && (*p != '-') && (*p != ' ')){
        p++;
    }

    if ((*p != '-') && (*p != ' ')) {
        ADDLOG_ERROR(LOG_FEATURE_IR, (char *)"IRSend cmnd not valid [%s] not like [NEC-0-1A] or [NEC 0 1A 1].", args);
        return CMD_RES_BAD_ARGUMENT;
    }

    int ournamelen = (p - args);
    int protocol = -1;
    for (int i = 0; i < numProtocols; i++){
        const char *name = ProtocolNames[i];
        int namelen = strlen(name);
        if (!my_strnicmp(name, args, namelen) && (ournamelen == namelen)){
            protocol = i;
            break;
        }
    }
	if (protocol == -1) {
		ADDLOG_ERROR(LOG_FEATURE_IR, "Unknown IR protocol in send!");
		protocol = 0;
	}

    p++;
    int addr = strtol(p, &p, 16);
    if ((*p != '-') && (*p != ' ')) {
        ADDLOG_ERROR(LOG_FEATURE_IR, (char *)"IRSend cmnd not valid [%s] not like [NEC-0-1A] or [NEC 0 1A 1].", args);
        return CMD_RES_BAD_ARGUMENT;
    }
    p++;
    int command = strtol(p, &p, 16);

    IRData data;
    memset(&data, 0, sizeof(data));
    int repeats = 0;

    if ((*p == '-') || (*p == ' ')) {
        p++;
        repeats = strtol(p, &p, 16);
    }
	int bits;
	if (protocol == SONY) {
		bits = 12;
	}
	else {
		bits = 0;
	}
	if ((*p == '-') || (*p == ' ')) {
		p++;
		bits = strtol(p, &p, 16);
	}


    data.protocol = (decode_type_t)protocol;
    data.address = addr;
    data.command = command;
    data.flags = 0;
	data.numberOfBits = bits;

    if (pIRsend){
		IrSender.SetParent(pIRsend);
        pIRsend->write(&data, (int_fast8_t) repeats);
        // add a 100ms delay after command
        // NOTE: this is NOT a delay here.  it adds 100ms 'space' in the TX queue
        pIRsend->delay(100);
        ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IR send %s protocol %d addr 0x%X cmd 0x%X repeats %d bits override %i", 
			args, (int)data.protocol, (int)data.address, (int)data.command, (int)repeats, (int)bits);
        return CMD_RES_OK;
    } else {
        ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IR NOT send (no IRsend running) %s protocol %d addr 0x%X cmd 0x%X repeats %d", args, (int)data.protocol, (int)data.address, (int)data.command, (int)repeats);
    }
    return CMD_RES_ERROR;
}

extern "C" commandResult_t IR_Enable(const void *context, const char *cmd, const char *args_in, int cmdFlags) {
    if (!args_in || !args_in[0]) {
        ADDLOG_ERROR(LOG_FEATURE_IR, (char *)"IREnable expects arguments");
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }

    char args[20];
    strncpy(args, args_in, 19);
    args[19] = 0;
    char *p = args;
    int enable = 1;
    if (!my_strnicmp(p, "RXTX", 4)){
        p += 4;
        if (*p == ' '){
            p++;
            if (*p){
                enable = atoi(p);
            }
        }
        gEnableIRSendWhilstReceive = enable;
        ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IREnable RX whilst TX enable set %d", enable);
        return CMD_RES_OK;
    }

    if (!my_strnicmp(p, "invert", 6)){
        // default normal.
        enable = 0;
        p += 6;
        if (*p == ' '){
            p++;
            if (*p){
                enable = atoi(p);
            }
        }
        gIRPinPolarity = enable;
        ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IREnable invert set %d", enable);
        return CMD_RES_OK;
    }
    

    // find length of first arg.
    while (*p && (*p != ' ')){
        p++;
    }

    int numProtocols = sizeof(ProtocolNames)/sizeof(*ProtocolNames);
    int ournamelen = (p - args);
    int protocol = -1;
    for (int i = 0; i < numProtocols; i++){
        const char *name = ProtocolNames[i];
        int namelen = strlen(name);
        if (!my_strnicmp(name, args, namelen) && (ournamelen == namelen)){
            protocol = i;
            break;
        }
    }
    if (*p == ' '){
        p++;
        if (*p){
            enable = atoi(p);
        }
    }

    uint32_t thisbit = (1 << protocol);
    if (protocol < 0){
        ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IREnable invalid protocol %s", args);
        return CMD_RES_BAD_ARGUMENT;
    } else {
        ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IREnable found protocol %s(%d), enable %d from %s, bitmask 0x%08X", ProtocolNames[protocol], protocol, enable, p, thisbit);
    }
    if (enable) {
        gIRProtocolEnable = gIRProtocolEnable | thisbit;
    } else {
        gIRProtocolEnable = gIRProtocolEnable & (~thisbit);
    }
    ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IREnable Protocol mask now 0x%08X", gIRProtocolEnable);
    return CMD_RES_OK;
    
}


// test routine to start IR RX and TX
// currently fixed pins for testing.
extern "C" void DRV_IR_Init(){
	ADDLOG_INFO(LOG_FEATURE_IR, (char *)"Log from extern C CPP");

	int pin = -1; //9;// PWM3/25
    int txpin = -1; //24;// PWM3/25

	// allow user to change them
	pin = PIN_FindPinIndexForRole(IOR_IRRecv,pin);
	txpin = PIN_FindPinIndexForRole(IOR_IRSend,txpin);

    if (ourReceiver){
        IRrecv *temp = ourReceiver;
        ourReceiver = NULL;
        delete temp;
    }
	ADDLOG_INFO(LOG_FEATURE_IR, (char *)"DRV_IR_Init: recv pin %i",pin);
    if ((pin > 0) || (txpin > 0)){
    } else {
        _timer_disable();
    }


    if (pin > 0){
        // setup IRrecv pin as input
        bk_gpio_config_input_pup((GPIO_INDEX)pin);

        ourReceiver = new IRrecv(pin);
        ourReceiver->start();
    }

    if (pIRsend){
        myIRsend *pIRsendTemp = pIRsend;
        pIRsend = NULL;
        delete pIRsendTemp;
    }

    if (txpin > 0){
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

	//cmddetail:{"name":"IRSend","args":"[PROT-ADDR-CMD-REP-BITS]",
	//cmddetail:"descr":"Sends IR commands in the form PROT-ADDR-CMD-REP-BITS, e.g. NEC-1-1A-0-0, note that -BITS is optional, it can be 0 for default one, so you can just do NEC-1-1A-0",
	//cmddetail:"fn":"IR_Send_Cmd","file":"driver/drv_ir.cpp","requires":"",
	//cmddetail:"examples":""}
            CMD_RegisterCommand("IRSend",IR_Send_Cmd, NULL);
	//cmddetail:{"name":"IREnable","args":"[Str][1or0]",
	//cmddetail:"descr":"Enable/disable aspects of IR.  IREnable RXTX 0/1 - enable Rx whilst Tx.  IREnable [protocolname] 0/1 - enable/disable a specified protocol",
	//cmddetail:"fn":"IR_Enable","file":"driver/drv_ir.cpp","requires":"",
	//cmddetail:"examples":""}
            CMD_RegisterCommand("IREnable", IR_Enable, NULL);
        }
    }
    if ((pin > 0) || (txpin > 0)){
        // both tx and rx need the interrupt
        _timerConfigForReceive();
        _timer_enable();
    }
}


// log the received IR
void PrintIRData(IRData *aIRDataPtr){
    ADDLOG_DEBUG(LOG_FEATURE_IR, (char *)"IR decode returned true, protocol %d", (int)aIRDataPtr->protocol);
    if (aIRDataPtr->protocol == UNKNOWN) {
#if defined(DECODE_HASH)
        ADDLOG_DEBUG(LOG_FEATURE_IR, (char *)" Hash=0x%X", (int)aIRDataPtr->decodedRawData);
#endif
        ADDLOG_DEBUG(LOG_FEATURE_IR, (char *)"%d bits (incl. gap and start) received", (int)((aIRDataPtr->rawDataPtr->rawlen + 1) / 2));
    } else {
#if defined(DECODE_DISTANCE)
        if(aIRDataPtr->protocol != PULSE_DISTANCE) {
#endif
        /*
         * New decoders have address and command
         */
        ADDLOG_DEBUG(LOG_FEATURE_IR, (char *)"Address=0x%X Command=0x%X", (int)aIRDataPtr->address, (int)aIRDataPtr->command);

        if (aIRDataPtr->flags & IRDATA_FLAGS_EXTRA_INFO) {
            ADDLOG_DEBUG(LOG_FEATURE_IR, (char *)" Extra=0x%X", (int)aIRDataPtr->extra);
        }

        if (aIRDataPtr->flags & IRDATA_FLAGS_PARITY_FAILED) {
            ADDLOG_DEBUG(LOG_FEATURE_IR, (char *)" Parity fail");
        }

        if (aIRDataPtr->flags & IRDATA_TOGGLE_BIT_MASK) {
            if (aIRDataPtr->protocol == NEC) {
                ADDLOG_DEBUG(LOG_FEATURE_IR, (char *)" Special repeat");
            } else {
                ADDLOG_DEBUG(LOG_FEATURE_IR, (char *)" Toggle=1");
            }
        }
#if defined(DECODE_DISTANCE)
        }
#endif
        if (aIRDataPtr->flags & (IRDATA_FLAGS_IS_AUTO_REPEAT | IRDATA_FLAGS_IS_REPEAT)) {
            if (aIRDataPtr->flags & IRDATA_FLAGS_IS_AUTO_REPEAT) {
                ADDLOG_DEBUG(LOG_FEATURE_IR, (char *)"Auto-Repeat");
            } else {
                ADDLOG_DEBUG(LOG_FEATURE_IR, (char *)"Repeat");
            }
            if (1) {
                ADDLOG_DEBUG(LOG_FEATURE_IR, (char *)" Gap %uus", (uint32_t)aIRDataPtr->rawDataPtr->rawbuf[0] * MICROS_PER_TICK);
            }
        }

        /*
         * Print raw data
         */
        if (!(aIRDataPtr->flags & IRDATA_FLAGS_IS_REPEAT) || aIRDataPtr->decodedRawData != 0) {
            ADDLOG_DEBUG(LOG_FEATURE_IR, (char *)" Raw-Data=0x%X", aIRDataPtr->decodedRawData);
            /*
             * Print number of bits processed
             */
            ADDLOG_DEBUG(LOG_FEATURE_IR, (char *)" %d bits", aIRDataPtr->numberOfBits);

            if (aIRDataPtr->flags & IRDATA_FLAGS_IS_MSB_FIRST) {
                ADDLOG_DEBUG(LOG_FEATURE_IR, (char *)" MSB first", aIRDataPtr->numberOfBits);
            } else {
                ADDLOG_DEBUG(LOG_FEATURE_IR, (char *)" LSB first", aIRDataPtr->numberOfBits);
            }
        }
    }
}


////////////////////////////////////////////////////
// this polls the IR receive to see off there was any IR received
extern "C" void DRV_IR_RunFrame(){
	// Debug-only check to see if the timer interrupt is running
    if (ir_counter){
        //ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IR counter: %u", ir_counter);
    }
    if (pIRsend){
        if (pIRsend->overflows){
            ADDLOG_DEBUG(LOG_FEATURE_IR, (char *)"##### IR send overflows %d", (int)pIRsend->overflows);
            pIRsend->resetsendqueue();
        } else {
            //ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IR send count %d remains %d currentus %d", (int)pIRsend->timecounttotal, (int)pIRsend->timecount, (int)pIRsend->currentsendtime);
        }
    }

    if (ourReceiver){
        if (ourReceiver->decode()) {
			const char *name = ProtocolNames[ourReceiver->decodedIRData.protocol];
            if (!(gIRProtocolEnable & (1 << (int)ourReceiver->decodedIRData.protocol))){
				ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IR decode ignore masked protocol %s (%d) - mask 0x%08X", name, (int)ourReceiver->decodedIRData.protocol, gIRProtocolEnable);
            }

			// 'UNKNOWN' protocol is by default disabled in flags
			// This is because I am getting a lot of 'UNKNOWN' spam with no IR signals in room
			if (((ourReceiver->decodedIRData.protocol != UNKNOWN) || 
                 (ourReceiver->decodedIRData.protocol == UNKNOWN && CFG_HasFlag(OBK_FLAG_IR_ALLOW_UNKNOWN))) &&
                 // only process if this protocol is enabled.  all by default.
                 (gIRProtocolEnable & (1 << (int)ourReceiver->decodedIRData.protocol))
               ) {


				char out[128];
				PrintIRData(&ourReceiver->decodedIRData);
				ADDLOG_DEBUG(LOG_FEATURE_IR, (char *)"IR decode returned true, protocol %s (%d)", name, (int)ourReceiver->decodedIRData.protocol);
                int repeat = 0;
                if (ourReceiver->decodedIRData.flags & (IRDATA_FLAGS_IS_AUTO_REPEAT | IRDATA_FLAGS_IS_REPEAT)) {
                    if (ourReceiver->decodedIRData.flags & IRDATA_FLAGS_IS_AUTO_REPEAT) {
                        repeat = 2;
                    } else {
                        repeat = 1;
                    }
                }

                if (ourReceiver->decodedIRData.protocol == UNKNOWN){
                    snprintf(out, sizeof(out), "IR_%s 0x%lX %d", name, (unsigned long)ourReceiver->decodedIRData.decodedRawData, repeat);
                } else {
                    snprintf(out, sizeof(out), "IR_%s 0x%X 0x%X %d (%i bits)", 
						name, ourReceiver->decodedIRData.address, ourReceiver->decodedIRData.command,
						repeat, ourReceiver->decodedIRData.numberOfBits);
                }
				// if user wants us to publish every received IR data, do it now
				if(CFG_HasFlag(OBK_FLAG_IR_PUBLISH_RECEIVED)) {

                    // another flag required?
                    int publishrepeats = 1;

                    if (publishrepeats || !repeat){
        				//ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IR MQTT publish %s", out);

                        uint32_t counter_in = ir_counter;
                        MQTT_PublishMain_StringString("ir",out, 0);
                        uint32_t counter_dur = ((ir_counter - counter_in)*50)/1000;
        				ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IR MQTT publish %s took %dms", out, counter_dur);
                    } else {
                        ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IR %s", out);
                    }
				} else {
            		ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IR %s", out);
                }

				if (CFG_HasFlag(OBK_FLAG_IR_PUBLISH_RECEIVED_IN_JSON)) {
					// {"IrReceived":{"Protocol":"RC_5","Bits":0x1,"Data":"0xC"}}
					// 
					snprintf(out, sizeof(out), "{\"IrReceived\":{\"Protocol\":\"%s\",\"Bits\":%i,\"Data\":\"0x%lX\"}}",
						name, (int)ourReceiver->decodedIRData.numberOfBits, (unsigned long)ourReceiver->decodedIRData.decodedRawData);
					MQTT_PublishMain_StringString("RESULT", out, OBK_PUBLISH_FLAG_FORCE_REMOVE_GET);
				}

				if(ourReceiver->decodedIRData.protocol != UNKNOWN) {
					snprintf(out, sizeof(out), "%X", ourReceiver->decodedIRData.command);
					int tgType = 0;
					switch(ourReceiver->decodedIRData.protocol)
					{
					case NEC:
						tgType = CMD_EVENT_IR_NEC;
						break;
					case SAMSUNG:
						tgType = CMD_EVENT_IR_SAMSUNG;
						break;
					case SHARP:
						tgType = CMD_EVENT_IR_SHARP;
						break;
					case RC5:
						tgType = CMD_EVENT_IR_RC5;
						break;
					case RC6:
						tgType = CMD_EVENT_IR_RC6;
						break;
					case SONY:
						tgType = CMD_EVENT_IR_SONY;
						break;
                    default:                        
                        break;
					}

                    // we should include repeat here?
                    // e.g. on/off button should not toggle on repeats, but up/down probably should eat them.
                    uint32_t counter_in = ir_counter;
					EventHandlers_FireEvent2(tgType,ourReceiver->decodedIRData.address,ourReceiver->decodedIRData.command);
                    uint32_t counter_dur = ((ir_counter - counter_in)*50)/1000;
      				ADDLOG_DEBUG(LOG_FEATURE_IR, (char *)"IR fire event took %dms", counter_dur);
				}
			}
			/*
			* !!!Important!!! Enable receiving of the next value,
			* since receiving has stopped after the end of the current received data packet.
			*/
			ourReceiver->resume(); // Enable receiving of the next value
        }
    }
}





#ifdef TEST_CPP
// routines to test C++
class cpptest2 {
    public:
        int initialised;
        cpptest2(){
        	// remove else static class may kill us!!!ADDLOG_INFO(LOG_FEATURE_IR, "Log from Class constructor");
            initialised = 42;
        };
        ~cpptest2(){
            initialised = 24;
        	ADDLOG_INFO(LOG_FEATURE_IR, (char *)"Log from Class destructor");
        }

        void print(){
        	ADDLOG_INFO(LOG_FEATURE_IR, (char *)"Log from Class %d", initialised);
        }
};

cpptest2 staticclass;

void cpptest(){
	ADDLOG_INFO(LOG_FEATURE_IR, (char *)"Log from CPP");
    cpptest2 test;
    test.print();
    cpptest2 *test2 = new cpptest2();
    test2->print();
	ADDLOG_INFO(LOG_FEATURE_IR, (char *)"Log from static class (is it initialised?):");
    staticclass.print();
}
#endif

#endif

