#if PLATFORM_BEKEN
// drv_ir_new.cpp - IRremoteESP8266

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
#undef send
//#define PROGMEM


//#define NO_LED_FEEDBACK_CODE 1

//typedef unsigned char uint_fast8_t;
typedef unsigned short uint16_t;

#define __FlashStringHelper char

// dummy functions
void noInterrupts() {}
void interrupts() {}

unsigned long millis() {
	return 0;
}
unsigned long micros() {
	return 0;
}


void delay(int n) {
	return;
}

void delayMicroseconds(int n) {
	return;
}

class Print {
public:
	void println(const char *p) {
		return;
	}
	void print(...) {
		return;
	}
};

Print Serial;




#define EXTERNAL_IR_TIMER_ISR

//////////////////////////////////////////
// our external timer interrupt stuff
// this will have already been done
#define TIMER_RESET_INTR_PENDING


// #  if defined(ISR)
// #undef ISR
// #  endif
// #define ISR void IR_ISR

// THIS function is defined in src/libraries/IRremoteESP8266/src/IRrecv.cpp
extern "C" void DRV_IR_ISR(UINT8 t);
extern void IR_ISR();

static UINT32 ir_chan = BKTIMER0;
static UINT32 ir_div = 1;
static UINT32 ir_periodus = 50;

void timerConfigForReceive() {
	// nothing here`
}

void _timerConfigForReceive() {
	ir_counter = 0;

	timer_param_t params = {
		(unsigned char)ir_chan,
		(unsigned char)ir_div, // div
		ir_periodus, // us
		DRV_IR_ISR
	};
	//GLOBAL_INT_DECLARATION();


	UINT32 res;
	// test what error we get with an invalid command
	res = sddev_control((char *)TIMER_DEV_NAME, -1, nullptr);

	if (res == 1) {
		ADDLOG_INFO(LOG_FEATURE_IR, (char *)"bk_timer already initialised");
	}
	else {
		ADDLOG_ERROR(LOG_FEATURE_IR, (char *)"bk_timer driver not initialised?");
		if ((int)res == -5) {
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

static void timer_enable() {
}
static void timer_disable() {
}
static void _timer_enable() {
	UINT32 res;
	res = sddev_control((char *)TIMER_DEV_NAME, CMD_TIMER_UNIT_ENABLE, &ir_chan);
	ADDLOG_INFO(LOG_FEATURE_IR, (char *)"ir timer enabled %u", res);
}
static void _timer_disable() {
	UINT32 res;
	res = sddev_control((char *)TIMER_DEV_NAME, CMD_TIMER_UNIT_DISABLE, &ir_chan);
	ADDLOG_INFO(LOG_FEATURE_IR, (char *)"ir timer disabled %u", res);
}

#define TIMER_ENABLE_RECEIVE_INTR timer_enable();
#define TIMER_DISABLE_RECEIVE_INTR timer_disable();

//////////////////////////////////////////

class SpoofIrReceiver {
public:
	static void restartAfterSend() {

	}
};

SpoofIrReceiver IrReceiver;

#include "../libraries/IRremoteESP8266/src/IRremoteESP8266.h"
#include "../libraries/IRremoteESP8266/src/IRsend.h"
#include "../libraries/IRremoteESP8266/src/IRrecv.h"
#include "../libraries/IRremoteESP8266/src/IRutils.h"
#ifdef ENABLE_IRAC
#include "../libraries/IRremoteESP8266/src/IRac.h"
#endif
#include "../libraries/IRremoteESP8266/src/IRproto.h"
#include "../libraries/IRremoteESP8266/src/digitalWriteFast.h"

extern "C" int PIN_GetPWMIndexForPinIndex(int pin);

// override aspects of sending for our own interrupt driven sends
// basically, IRsend calls mark(us) and space(us) to send.
// we simply note the numbers into a rolling buffer, assume the first is a mark()
// and then every 50us service the rolling buffer, changing the PWM from 0 duty to 50% duty
// appropriately.
#define SEND_MAXBITS 128

class myIRsend : public IRsend {
public:
	myIRsend(uint_fast8_t aSendPin) :IRsend(aSendPin) {
		our_us = 0;
		our_ms = 0;
		resetsendqueue();
	}
	~myIRsend() { }


	uint32_t millis() {
		return our_ms;
	}

	void delay(long int ms) {
		// add a pure delay to our queue
		space(ms * 1000);
	}

	uint16_t mark(uint16_t aMarkMicros) {
		// sends a high for aMarkMicros
		uint32_t newtimein = (timein + 1) % (SEND_MAXBITS * 2);
		if (newtimein != timeout) {
			// store mark bits in highest +ve bit of count
			times[timein] = aMarkMicros | 0x10000000;
			timein = newtimein;
			timecount++;
			timecounttotal++;
		}
		else {
			overflows++;
		}
		return 1;
	}

	void space(uint32_t aMarkMicros) {
		// sends a low for aMarkMicros
		uint32_t newtimein = (timein + 1) % (SEND_MAXBITS * 2);
		if (newtimein != timeout) {
			times[timein] = aMarkMicros;
			timein = newtimein;
			timecount++;
			timecounttotal++;
		}
		else {
			overflows++;
		}
	}

	void enableIROut(uint32_t freq, uint8_t duty=50) {
		//uint_fast8_t aFrequencyKHz
		if (freq < 1000)  // Were we given kHz? Supports the old call usage.
			freq *= 1000;
		ADDLOG_INFO(LOG_FEATURE_IR, (char *)"enableIROut %d freq %d duty",(int)freq, (int)duty);
		if(duty<1)
			duty=1;
		if(duty>100)
			duty=100;
		// just setup variables for use in ISR
		//pwmfrequency = ((uint32_t)aFrequencyKHz) * 1000;
		pwmperiod = (26000000 / freq);
		pwmduty = pwmperiod / (100/duty);
		
		
#if PLATFORM_BK7231N
		bk_pwm_update_param((bk_pwm_t)this->pwmIndex, this->pwmperiod, pwmduty, 0, 0);
#else
		bk_pwm_update_param((bk_pwm_t)this->pwmIndex, this->pwmperiod, pwmduty);
#endif
	}

	void resetsendqueue() {
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

	int32_t getsendqueue() {
		int32_t val = 0;
		if (timein != timeout) {
			val = times[timeout];
			timeout = (timeout + 1) % (SEND_MAXBITS * 2);
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
extern "C" void DRV_IR_ISR(UINT8 t) {
	int sending = 0;
	if (pIRsend && (pIRsend->pwmIndex >= 0)) {
		pIRsend->our_us += 50;
		if (pIRsend->our_us > 1000) {
			pIRsend->our_ms++;
			pIRsend->our_us -= 1000;
		}

		int pinval = 0;
		if (pIRsend->currentsendtime) {
			sending = 1;
			pIRsend->currentsendtime -= ir_periodus;
			if (pIRsend->currentsendtime <= 0) {
				int32_t remains = pIRsend->currentsendtime;
				int32_t newtime = pIRsend->getsendqueue();
				if (0 == newtime) {
					// if it was the last one
					pIRsend->currentsendtime = 0;
					pIRsend->currentbitval = 0;
				}
				else {
					// we got a new time
					// store mark bits in highest +ve bit of count
					pIRsend->currentbitval = (newtime & 0x10000000) ? 1 : 0;
					pIRsend->currentsendtime = (newtime & 0xfffffff);
					// adjust the us value to keep the running accuracy
					// and avoid a running error?
					// note remains is -ve
					pIRsend->currentsendtime += remains;
				}
			}
		}
		else {
			int32_t newtime = pIRsend->getsendqueue();
			if (!newtime) {
				pIRsend->currentsendtime = 0;
				pIRsend->currentbitval = 0;
			}
			else {
				sending = 1;
				pIRsend->currentsendtime = (newtime & 0xfffffff);
				pIRsend->currentbitval = (newtime & 0x10000000) ? 1 : 0;
			}
		}
		pinval = pIRsend->currentbitval;

		uint32_t duty = pIRsend->pwmduty;
		if (!pinval) {
			if (gIRPinPolarity) {
				duty = pIRsend->pwmperiod;
			}
			else {
				duty = 0;
			}
		}
#if PLATFORM_BK7231N
		bk_pwm_update_param((bk_pwm_t)pIRsend->pwmIndex, pIRsend->pwmperiod, duty, 0, 0);
#else
		bk_pwm_update_param((bk_pwm_t)pIRsend->pwmIndex, pIRsend->pwmperiod, duty);
#endif
	}

	// is someone really wants rx and TX at the same time, then allow it.
	if (gEnableIRSendWhilstReceive) {
		sending = 0;
	}

	// don't receive if we are currently sending
	if (ourReceiver && !sending){
		IR_ISR();
	}
	ir_counter++;
}


extern "C" commandResult_t IR_Send_Cmd(const void *context, const char *cmd, const char *args_in, int cmdFlags) {
	if (!args_in) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	char args[128];
	strncpy(args, args_in, sizeof(args) - 1);
	args[sizeof(args) - 1] = 0;

	// split arg at hyphen;
	char *p = args;
	while (*p && (*p != '-') && (*p != ' ')) {
		p++;
	}

	if ((*p != '-') && (*p != ' ')) {
		// try to decode "new" format, separated by comma
		// the format is bits,0xDATA[,repeat]
		char *p = args;
		while (*p && (*p != ',')) {
			p++;
		}
		if(*p==',')
		{
			*p='\0';
			decode_type_t protocol = strToDecodeType(args);
			p++;
			char *_bits=p;
			while (*p && (*p != ',')) {
				p++;
			}
			if(*p==',')
			{
				*p='\0';
				uint16_t bits = (uint16_t)strtol(_bits,NULL,10);
				p++;
				if(bits<=64)
				{
					char *_data=p;
					uint64_t data =  strtoll(_data,&p,16);
					if(protocol!=decode_type_t::UNKNOWN)
					{
						int repeats=1;
						if(*p==',')
							repeats=strtol(p+1,NULL,10);
						
						if( pIRsend->send(protocol,data,bits,repeats) )
						{
							pIRsend->delay(100);
							ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IR send %s: protocol %d bits %d data 0x%llX repeats %d", args, (int)protocol, (int)bits, (long long int)data, (int)repeats);
							return CMD_RES_OK;
						} else {
							ADDLOG_ERROR(LOG_FEATURE_IR, (char *)"IR can't send %s: protocol %d bits data 0x%llX repeats %d", args, (int)protocol, (long long int)data, (int)repeats);
							return CMD_RES_BAD_ARGUMENT;
						}
					}
				} else {
					// TODO: implement longer protocols
					ADDLOG_ERROR(LOG_FEATURE_IR, (char *)"IRSend currently only protocol with up to 64bits are supported", args);
					return CMD_RES_BAD_ARGUMENT;
				}
			} 
		}
		ADDLOG_ERROR(LOG_FEATURE_IR, (char *)"IRSend cmnd not valid [%s] not like [NEC-0-1A] or [NEC 0 1A 1] or [NEC,bits,0xDATA,[repeat]]", args);
 		return CMD_RES_BAD_ARGUMENT;
	}

	*p='\0';
	decode_type_t protocol = strToDecodeType(args);
	if(hasACState(protocol))
	{
		ADDLOG_ERROR(LOG_FEATURE_IR, (char *)"IRSend can't send AC commands", args);
		return CMD_RES_BAD_ARGUMENT;
	}
	p++;
	int addr = strtol(p, &p, 16);
	if ((*p != '-') && (*p != ' ')) {
		ADDLOG_ERROR(LOG_FEATURE_IR, (char *)"IRSend cmnd not valid [%s] not like [NEC-0-1A] or [NEC 0 1A 1].", args);
		return CMD_RES_BAD_ARGUMENT;
	}
	p++;
	int command = strtol(p, &p, 16);

	int repeats = 0;

	if ((*p == '-') || (*p == ' ')) {
		p++;
		repeats = strtol(p, &p, 16);
	}

	if (pIRsend) {
		bool success = true;  // Assume success.

		switch(protocol)
		{
			case decode_type_t::RC5:
				pIRsend->sendRC5((uint64_t)pIRsend->encodeRC5(addr,command));
				break;
			case decode_type_t::RC6:
				pIRsend->sendRC6((uint64_t)pIRsend->encodeRC6(addr,command));
				break;
			case decode_type_t::NEC:
				pIRsend->sendNEC((uint64_t)pIRsend->encodeNEC(addr,command));
				break;
			case decode_type_t::PANASONIC:
				pIRsend->sendPanasonic((uint16_t)addr,(uint32_t)command);
				break;
			case decode_type_t::JVC:
				pIRsend->sendJVC((uint64_t)pIRsend->encodeJVC(addr,command));
				break;
			case decode_type_t::SAMSUNG:
				pIRsend->sendSAMSUNG((uint64_t)pIRsend->encodeSAMSUNG(addr,command));
				break;
			case decode_type_t::LG:
				pIRsend->sendLG((uint64_t)pIRsend->encodeLG(addr,command));
				break;
			default:
				ADDLOG_ERROR(LOG_FEATURE_IR, (char *)"IR send %s protocol not supported", args);
				return CMD_RES_ERROR;
				break;
		};

		// add a 100ms delay after command
		// NOTE: this is NOT a delay here.  it adds 100ms 'space' in the TX queue
		pIRsend->delay(100);

		ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IR send %s protocol %d addr 0x%X cmd 0x%X repeats %d", args, (int)protocol, (int)addr, (int)command, (int)repeats);
		return CMD_RES_OK;
	}
	else {
		ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IR NOT send (no IRsend running) %s protocol %d addr 0x%X cmd 0x%X repeats %d", args, (int)protocol, (int)addr, (int)command, (int)repeats);
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
	if (!my_strnicmp(p, "RXTX", 4)) {
		p += 4;
		if (*p == ' ') {
			p++;
			if (*p) {
				enable = atoi(p);
			}
		}
		gEnableIRSendWhilstReceive = enable;
		ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IREnable RX whilst TX enable set %d", enable);
		return CMD_RES_OK;
	}

	if (!my_strnicmp(p, "invert", 6)) {
		// default normal.
		enable = 0;
		p += 6;
		if (*p == ' ') {
			p++;
			if (*p) {
				enable = atoi(p);
			}
		}
		gIRPinPolarity = enable;
		ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IREnable invert set %d", enable);
		return CMD_RES_OK;
	}


	// find length of first arg.
	while (*p && (*p != ' ')) {
		p++;
	}

	//int numProtocols = sizeof(ProtocolNames)/sizeof(*ProtocolNames);
	#if 0 // number of protocols now is 125 
	// TODO: reimpleemnt this using bigger mask
	int numProtocols = 0;
	int ournamelen = (p - args);
	int protocol = -1;
	for (int i = 0; i < numProtocols; i++) {
		const char *name = "Unknown"; //= ProtocolNames[i];
		int namelen = strlen(name);
		if (!my_strnicmp(name, args, namelen) && (ournamelen == namelen)) {
			protocol = i;
			break;
		}
	}
	if (*p == ' ') {
		p++;
		if (*p) {
			enable = atoi(p);
		}
	}

	uint32_t thisbit = (1 << protocol);
	if (protocol < 0) {
		ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IREnable invalid protocol %s", args);
		return CMD_RES_BAD_ARGUMENT;
	}
	else {
		//ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IREnable found protocol %s(%d), enable %d from %s, bitmask 0x%08X", ProtocolNames[protocol], protocol, enable, p, thisbit);
	}
	if (enable) {
		gIRProtocolEnable = gIRProtocolEnable | thisbit;
	}
	else {
		gIRProtocolEnable = gIRProtocolEnable & (~thisbit);
	}
	ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IREnable Protocol mask now 0x%08X", gIRProtocolEnable);
	#endif //TODO
	return CMD_RES_OK;
}


extern "C" commandResult_t IR_Param(const void *context, const char *cmd, const char *args_in, int cmdFlags) {
	if (!args_in || !args_in[0]) {
		ADDLOG_ERROR(LOG_FEATURE_IR, (char *)"IRParam expects two arguments");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	if(!ourReceiver)
	{
		ADDLOG_ERROR(LOG_FEATURE_IR, (char *)"IRParam: IR reciever disabled");
		return CMD_RES_BAD_ARGUMENT;
	}

	// Set higher if you get lots of random short UNKNOWN messages when nothing
	// should be sending a message.
	// Set lower if you are sure your setup is working, but it doesn't see messages
	// from your device. (e.g. Other IR remotes work.)
	// NOTE: Set this value very high to effectively turn off UNKNOWN detection.	
	int kMinUnknownSize = 12;

	// How much percentage lee way do we give to incoming signals in order to match
	// it?
	// e.g. +/- 25% (default) to an expected value of 500 would mean matching a
	//      value between 375 & 625 inclusive.
	// Note: Default is 25(%). Going to a value >= 50(%) will cause some protocols
	//       to no longer match correctly. In normal situations you probably do not
	//       need to adjust this value. Typically that's when the library detects
	//       your remote's message some of the time, but not all of the time.
	int kTolerancePercentage = 25;  // kTolerance is normally 25%

	int res = sscanf(args_in, "%d %d", &kMinUnknownSize, &kTolerancePercentage);

	if(res!=2)
	{
		ADDLOG_ERROR(LOG_FEATURE_IR, (char *)"IRParam invalid parameters %s", args_in);
		return CMD_RES_BAD_ARGUMENT;
	}
	ourReceiver->setUnknownThreshold(kMinUnknownSize);
	ourReceiver->setTolerance(kTolerancePercentage);

	ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IRParam MinUnknownSize: %d  Noice tolerance: %d%%", kMinUnknownSize,kTolerancePercentage);
	return CMD_RES_OK;
}



#ifdef ENABLE_IRAC
extern "C" commandResult_t IR_AC_Cmd(const void *context, const char *cmd, const char *args_in, int cmdFlags) {
	if (!args_in) return CMD_RES_NOT_ENOUGH_ARGUMENTS;

	char args[64];
	strncpy(args, args_in, sizeof(args) - 1);
	args[sizeof(args) - 1] = 0;

	// split arg at hyphen;
	char *p = args;
	while (*p && (*p != '-') && (*p != ' ')) {
		p++;
	}
	int ournamelen = (p - args);
	if ((*p != '-') && (*p != ' ')) {
		ADDLOG_ERROR(LOG_FEATURE_IR, (char *)"IRAC cmnd not valid [%s] ", args);
		return CMD_RES_BAD_ARGUMENT;
	}
	//	decode_type_t protocol = strToDecodeType(args);

	ADDLOG_ERROR(LOG_FEATURE_IR, (char *)"IRAC cmnd not implemented yet", args);

	return CMD_RES_OK;
}
#endif //ENABLE_IRAC


// test routine to start IR RX and TX
// currently fixed pins for testing.
extern "C" void DRV_IR_Init() {
	ADDLOG_INFO(LOG_FEATURE_IR, (char *)"Log from extern C CPP");

	int pin = -1; //9;// PWM3/25
	int txpin = -1; //24;// PWM3/25

	// allow user to change them
	pin = PIN_FindPinIndexForRole(IOR_IRRecv, pin);
	txpin = PIN_FindPinIndexForRole(IOR_IRSend, txpin);

	if (ourReceiver){
	     IRrecv *temp = ourReceiver;
	     ourReceiver = NULL;
	     delete temp;
	 }
	ADDLOG_INFO(LOG_FEATURE_IR, (char *)"DRV_IR_Init: recv pin %i", pin);
	if ((pin > 0) || (txpin > 0)) {
	}
	else {
		_timer_disable();
	}

	if (pin > 0) {
		// setup IRrecv pin as input
		//bk_gpio_config_input_pup((GPIO_INDEX)pin); // enabled by enableIRIn

		//TODO: we should specify buffer size (now set to 1024), timeout (now 90ms) and tolerance 
		 ourReceiver = new IRrecv(pin);
		 ourReceiver->enableIRIn(true);// try with pullup
	}

	if (pIRsend) {
		myIRsend *pIRsendTemp = pIRsend;
		pIRsend = NULL;
		delete pIRsendTemp;
	}

	if (txpin > 0) {
		int pwmIndex = PIN_GetPWMIndexForPinIndex(txpin);
		// is this pin capable of PWM?
		if (pwmIndex != -1) {
			uint32_t pwmfrequency = 38000;
			uint32_t period = (26000000 / pwmfrequency);
			uint32_t duty = period / 2;
#if PLATFORM_BK7231N
			// OSStatus bk_pwm_initialize(bk_pwm_t pwm, uint32_t frequency, uint32_t duty_cycle);
			bk_pwm_initialize((bk_pwm_t)pwmIndex, period, duty, 0, 0);
#else
			bk_pwm_initialize((bk_pwm_t)pwmIndex, period, duty);
#endif
			bk_pwm_start((bk_pwm_t)pwmIndex);
			myIRsend *pIRsendTemp = new myIRsend((uint_fast8_t)txpin);
			pIRsendTemp->resetsendqueue();
			pIRsendTemp->enableIROut(pwmfrequency,50);
			pIRsendTemp->pwmIndex = pwmIndex;
			pIRsendTemp->pwmduty = duty;
			pIRsendTemp->pwmperiod = period;

			pIRsend = pIRsendTemp;
			//bk_pwm_stop((bk_pwm_t)pIRsend->pwmIndex);

			//cmddetail:{"name":"IRSend","args":"[PROT-ADDR-CMD-REP]",
			//cmddetail:"descr":"Sends IR commands in the form PROT-ADDR-CMD-REP, e.g. NEC-1-1A-0",
			//cmddetail:"fn":"IR_Send_Cmd","file":"driver/drv_ir.cpp","requires":"",
			//cmddetail:"examples":""}
			CMD_RegisterCommand("IRSend", IR_Send_Cmd, NULL);
			//cmddetail:{"name":"IRAC","args":"[TODO]",
			//cmddetail:"descr":"Sends IR commands for HVAC control (TODO)",
			//cmddetail:"fn":"IR_AC_Cmd","file":"driver/drv_ir.cpp","requires":"",
			//cmddetail:"examples":""}
			#ifdef ENABLE_IRAC
			CMD_RegisterCommand("IRAC", IR_AC_Cmd, NULL);
			#endif //ENABLE_IRAC
			//cmddetail:{"name":"IREnable","args":"[Str][1or0]",
			//cmddetail:"descr":"Enable/disable aspects of IR.  IREnable RXTX 0/1 - enable Rx whilst Tx.  IREnable [protocolname] 0/1 - enable/disable a specified protocol",
			//cmddetail:"fn":"IR_Enable","file":"driver/drv_ir.cpp","requires":"",
			//cmddetail:"examples":""}
			CMD_RegisterCommand("IREnable",IR_Enable, NULL);
			//cmddetail:{"name":"IRParam","args":"[MinSize] [Noise Threshold]",
			//cmddetail:"descr":"Set minimal size of the message and noise threshold",
			//cmddetail:"fn":"IR_Enable","file":"driver/drv_ir.cpp","requires":"",
			//cmddetail:"examples":""}
			CMD_RegisterCommand("IRParam",IR_Param, NULL);
		}
	}
	if ((pin > 0) || (txpin > 0)) {
		// both tx and rx need the interrupt
		_timerConfigForReceive();
		_timer_enable();
	}
}


void dump(decode_results *results) {
	// Dumps out the decode_results structure.
	// Call this after IRrecv::decode()
	ADDLOG_INFO(LOG_FEATURE_IR, resultToHumanReadableBasic(results).c_str());

	#ifdef ENABLE_IRAC
	if (hasACState(results->decode_type))
	{
		ADDLOG_INFO(LOG_FEATURE_IR, IRAcUtils::resultAcToString(results).c_str());
	}
	#endif
}



////////////////////////////////////////////////////
// this polls the IR receive to see if there was any IR received
extern "C" void DRV_IR_RunFrame() {
	// Debug-only check to see if the timer interrupt is running
	if (ir_counter) {
		//ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IR counter: %u", ir_counter);
	}
	if (pIRsend) {
		if (pIRsend->overflows) {
			ADDLOG_DEBUG(LOG_FEATURE_IR, (char *)"##### IR send overflows %d", (int)pIRsend->overflows);
			pIRsend->resetsendqueue();
		}
		else {
			//ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IR send count %d remains %d currentus %d", (int)pIRsend->timecounttotal, (int)pIRsend->timecount, (int)pIRsend->currentsendtime);
		}
	}


	if (ourReceiver) {
		decode_results results;
		if (ourReceiver->decode(&results)) {
			// TODO: find a better way?
			String proto_name = typeToString(results.decode_type, results.repeat).c_str();

			#if 0 // TODO: implement different masking
			if (!(gIRProtocolEnable & (1 << (int)results.decode_type))) {
				ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IR decode ignore masked protocol %s (%d) - mask 0x%08X", proto_name.c_str(), (int)results.decode_type, gIRProtocolEnable);
			}
			#endif

			//dump(&results);
			// 'UNKNOWN' protocol is by default disabled in flags
			// This is because I am getting a lot of 'UNKNOWN' spam with no IR signals in room
			if (((results.decode_type != decode_type_t::UNKNOWN) ||
				(results.decode_type == decode_type_t::UNKNOWN && CFG_HasFlag(OBK_FLAG_IR_ALLOW_UNKNOWN))) //&&
				// only process if this protocol is enabled.  all by default.
				//(gIRProtocolEnable & (1 << (int)results.decode_type)
				) {
				String lastIrReceived = String((int)results.decode_type, 16) + "," + resultToHexidecimal(&results);

				if (!hasACState(results.decode_type))
					lastIrReceived += "," + String((int)results.bits);
				else
					ADDLOG_INFO(LOG_FEATURE_IR, "Recieved AC code:%s",proto_name.c_str());

				char out[128];

				int repeat = results.repeat?0:1; // not sure how to deal with this

				if (results.decode_type == decode_type_t::UNKNOWN) {
					//snprintf(out, sizeof(out), "IR_RAW 0x%lX %d", (unsigned long)results.decodedRawData, repeat);
					snprintf(out, sizeof(out), "IR %s %s", "Unknown", lastIrReceived.c_str());
					ADDLOG_INFO(LOG_FEATURE_IR, (char *)out);
				}
				else if (!hasACState(results.decode_type)) {
					snprintf(out, sizeof(out), "IR %s %lX %lX %d", proto_name.c_str(), (long int)results.address, (long int)results.command, repeat);
					ADDLOG_INFO(LOG_FEATURE_IR, (char *)out);
					// show new format too
					snprintf(out, sizeof(out), "IR %s,%d,%s", proto_name.c_str(), (int)results.bits, resultToHexidecimal(&results).c_str());
					ADDLOG_INFO(LOG_FEATURE_IR, (char *)out);
				} else {
					#ifdef ENABLE_IRAC
					String description = IRAcUtils::resultAcToString(&results);
					ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IRAC %s", description.c_str());
					#endif //ENABLE_IRAC
				}
				// if user wants us to publish every received IR data, do it now
				if (CFG_HasFlag(OBK_FLAG_IR_PUBLISH_RECEIVED)) {

					// another flag required?
					int publishrepeats = 1;

					if (publishrepeats || !repeat) {
						//ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IR MQTT publish %s", out);

						uint32_t counter_in = ir_counter;
						MQTT_PublishMain_StringString("ir", lastIrReceived.c_str(), 0);
						uint32_t counter_dur = ((ir_counter - counter_in) * 50) / 1000;
						ADDLOG_INFO(LOG_FEATURE_IR, (char *)"IR MQTT publish %s took %dms", out, counter_dur);
					}
					else {
						ADDLOG_INFO(LOG_FEATURE_IR, (char *)out);
					}
				}

				if (CFG_HasFlag(OBK_FLAG_IR_PUBLISH_RECEIVED_IN_JSON)) {
					// {"IrReceived":{"Protocol":"RC_5","Bits":0x1,"Data":"0xC"}}
					//
					String _data=resultToHexidecimal(&results);
					snprintf(out, sizeof(out), "{\"IrReceived\":{\"Protocol\":\"%s\",\"Bits\":%i,\"Data\":\"%s\"}}",
						proto_name.c_str(), (int)results.bits, _data.c_str());
					MQTT_PublishMain_StringString("RESULT", out, OBK_PUBLISH_FLAG_FORCE_REMOVE_GET);
				}

				if (results.decode_type != decode_type_t::UNKNOWN) {
					snprintf(out, sizeof(out), "%X", results.command);
					int tgType = 0;
					switch (results.decode_type)
					{
					case decode_type_t::NEC:
						tgType = CMD_EVENT_IR_NEC;
						break;
					case decode_type_t::SAMSUNG:
						tgType = CMD_EVENT_IR_SAMSUNG;
						break;
					case decode_type_t::SHARP:
						tgType = CMD_EVENT_IR_SHARP;
						break;
					case decode_type_t::RC5:
						tgType = CMD_EVENT_IR_RC5;
						break;
					case decode_type_t::RC6:
						tgType = CMD_EVENT_IR_RC6;
						break;
					case decode_type_t::SONY:
						tgType = CMD_EVENT_IR_SONY;
						break;
					default:
						break;
					}

					// we should include repeat here?
					// e.g. on/off button should not toggle on repeats, but up/down probably should eat them.
					uint32_t counter_in = ir_counter;
					EventHandlers_FireEvent2(tgType, results.address, results.command);
					uint32_t counter_dur = ((ir_counter - counter_in) * 50) / 1000;
					ADDLOG_DEBUG(LOG_FEATURE_IR, (char *)"IR fire event took %dms", counter_dur);
				}
			} else {
				ADDLOG_INFO(LOG_FEATURE_IR, "Recieved Unknown IR ");
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
	cpptest2() {
		// remove else static class may kill us!!!ADDLOG_INFO(LOG_FEATURE_IR, "Log from Class constructor");
		initialised = 42;
	};
	~cpptest2() {
		initialised = 24;
		ADDLOG_INFO(LOG_FEATURE_IR, (char *)"Log from Class destructor");
	}

	void print() {
		ADDLOG_INFO(LOG_FEATURE_IR, (char *)"Log from Class %d", initialised);
	}
};

cpptest2 staticclass;

void cpptest() {
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

