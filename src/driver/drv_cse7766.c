#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"

static float CSE7766_PREF = 598;
static float CSE7766_UREF = 15188;
static float CSE7766_IREF = 251210;

static int raw_unscaled_voltage;
static int raw_unscaled_current;
static int raw_unscaled_power;
static int raw_unscaled_freq;


#define CSE7766_BAUD_RATE 4800


// startDriver CSE7766
int CSE7766_TryToGetNextCSE7766Packet() {
	int cs;
	int i;
	int c_garbage_consumed = 0;
	byte a;
	byte checksum;
	int CSE7766_PACKET_LEN = 24;

	cs = UART_GetDataSize();


	if(cs < CSE7766_PACKET_LEN) {
		return 0;
	}
	// skip garbage data (should not happen)
	while(cs > 0) {
		a = UART_GetNextByte(1);
		if(a != 0x5A) {
			UART_ConsumeBytes(1);
			c_garbage_consumed++;
			cs--;
		} else {
			break;
		}
	}
	if(c_garbage_consumed > 0){
		addLogAdv(LOG_INFO, LOG_FEATURE_BL09XX,"Consumed %i unwanted non-header byte in CSE7766 buffer\n", c_garbage_consumed);
	}
	if(cs < CSE7766_PACKET_LEN) {
		return 0;
	}
	a = UART_GetNextByte(1);
	if(a != 0x5A) {
		return 0;
	}
	checksum = 0;

	for(i = 2; i < CSE7766_PACKET_LEN-1; i++) {
		checksum += UART_GetNextByte(i);
	}

#if 1
	{
		char buffer_for_log[128];
		char buffer2[32];
		buffer_for_log[0] = 0;
		for(i = 0; i < CSE7766_PACKET_LEN; i++) {
			sprintf(buffer2,"%02X ",UART_GetNextByte(i));
			strcat_safe(buffer_for_log,buffer2,sizeof(buffer_for_log));
		}
		addLogAdv(LOG_INFO, LOG_FEATURE_BL09XX,"CSE7766 received: %s\n", buffer_for_log);
	}
#endif
	if(checksum != UART_GetNextByte(CSE7766_PACKET_LEN-1)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_BL09XX,"Skipping packet with bad checksum %02X wanted %02X\n",checksum,UART_GetNextByte(CSE7766_PACKET_LEN-1));
		UART_ConsumeBytes(CSE7766_PACKET_LEN);
		return 1;
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_BL09XX,"CSE checksum ok");

	/*
	//startDriver CSE7766
	raw_unscaled_current = (UART_GetNextByte(3) << 16) | (UART_GetNextByte(2) << 8) | UART_GetNextByte(1);
	raw_unscaled_voltage = (UART_GetNextByte(6) << 16) | (UART_GetNextByte(5) << 8) | UART_GetNextByte(4);
	raw_unscaled_power = (UART_GetNextByte(12) << 24) | (UART_GetNextByte(11) << 16) | (UART_GetNextByte(10) << 8);
	raw_unscaled_power = (raw_unscaled_power >> 8);

	raw_unscaled_freq = (UART_GetNextByte(17) << 8) | UART_GetNextByte(16);

	// those are not values like 230V, but unscaled
	//addLogAdv(LOG_INFO, LOG_FEATURE_BL09XX,"Unscaled current %d, voltage %d, power %d, freq %d\n", raw_unscaled_current, raw_unscaled_voltage,raw_unscaled_power,raw_unscaled_freq);

	// those are final values, like 230V
	{
		float power, voltage, current;
		power = (raw_unscaled_power / CSE7766_PREF);
		voltage = (raw_unscaled_voltage / CSE7766_UREF);
		current = (raw_unscaled_current / CSE7766_IREF);

		BL_ProcessUpdate(voltage,current,power);
	}

*/
#if 0
	{
		char res[128];
		// V=245.107925,I=109.921143,P=0.035618
		sprintf(res,"V=%f,I=%f,P=%f\n",lastReadings[OBK_VOLTAGE],lastReadings[OBK_CURRENT],lastReadings[OBK_POWER]);
		addLogAdv(LOG_INFO, LOG_FEATURE_BL09XX,res );
	}
#endif

	UART_ConsumeBytes(CSE7766_PACKET_LEN);

	return CSE7766_PACKET_LEN;
}

void CSE7766_SendRequest() {
	//UART_InitUART(CSE7766_BAUD_RATE);
	//UART_SendByte(CSE7766_READ_COMMAND);
	//UART_SendByte(0xAA);
}
int CSE7766_PowerSet(const void *context, const char *cmd, const char *args, int cmdFlags) {
	float realPower;

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_BL09XX,"This command needs one argument");
		return 1;
	}
	realPower = atof(args);
	CSE7766_PREF = raw_unscaled_power / realPower;
	{
		char dbg[128];
		sprintf(dbg,"CurrentSet: you gave %f, set ref to %f\n", realPower, CSE7766_PREF);
		addLogAdv(LOG_INFO, LOG_FEATURE_BL09XX,dbg);
	}
	return 0;
}
int CSE7766_PowerRef(const void *context, const char *cmd, const char *args, int cmdFlags) {

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_BL09XX,"This command needs one argument");
		return 1;
	}
	CSE7766_PREF = atof(args);
	return 0;
}
int CSE7766_CurrentRef(const void *context, const char *cmd, const char *args, int cmdFlags) {

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_BL09XX,"This command needs one argument");
		return 1;
	}
	CSE7766_IREF = atof(args);
	return 0;
}
int CSE7766_VoltageRef(const void *context, const char *cmd, const char *args, int cmdFlags) {

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_BL09XX,"This command needs one argument");
		return 1;
	}
	CSE7766_UREF = atof(args);
	return 0;
}
int CSE7766_VoltageSet(const void *context, const char *cmd, const char *args, int cmdFlags) {
	float realV;

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_BL09XX,"This command needs one argument");
		return 1;
	}
	realV = atof(args);
	CSE7766_UREF = raw_unscaled_voltage / realV;
	{
		char dbg[128];
		sprintf(dbg,"CurrentSet: you gave %f, set ref to %f\n", realV, CSE7766_UREF);
		addLogAdv(LOG_INFO, LOG_FEATURE_BL09XX,dbg);
	}

	return 0;
}
int CSE7766_CurrentSet(const void *context, const char *cmd, const char *args, int cmdFlags) {
	float realI;

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_BL09XX,"This command needs one argument");
		return 1;
	}
	realI = atof(args);
	CSE7766_IREF = raw_unscaled_current / realI;
	{
		char dbg[128];
		sprintf(dbg,"CurrentSet: you gave %f, set ref to %f\n", realI, CSE7766_IREF);
		addLogAdv(LOG_INFO, LOG_FEATURE_BL09XX,dbg);
	}
	return 0;
}
void CSE7766_Init() {

	UART_InitUART(CSE7766_BAUD_RATE);
	UART_InitReceiveRingBuffer(512);
	CMD_RegisterCommand("PowerSet","",CSE7766_PowerSet, "Sets current power value for calibration", NULL);
	CMD_RegisterCommand("VoltageSet","",CSE7766_VoltageSet, "Sets current V value for calibration", NULL);
	CMD_RegisterCommand("CurrentSet","",CSE7766_CurrentSet, "Sets current I value for calibration", NULL);
	CMD_RegisterCommand("PREF","",CSE7766_PowerRef, "Sets the calibration multiplier", NULL);
	CMD_RegisterCommand("VREF","",CSE7766_VoltageRef, "Sets the calibration multiplier", NULL);
	CMD_RegisterCommand("IREF","",CSE7766_CurrentRef, "Sets the calibration multiplier", NULL);
}
void CSE7766_RunFrame() {
	int len;

	//addLogAdv(LOG_INFO, LOG_FEATURE_BL09XX,"UART buffer size %i\n", UART_GetDataSize());

	len = CSE7766_TryToGetNextCSE7766Packet();
	if(len > 0) {

	} else {
	//	CSE7766_SendRequest();
	}
}

