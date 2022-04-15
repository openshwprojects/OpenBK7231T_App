#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_bl0942.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"

float BL0942_PREF = 598;
float BL0942_UREF = 15188;
float BL0942_IREF = 251210;

int raw_unscaled_voltage;
int raw_unscaled_current;
int raw_unscaled_power;
int raw_unscaled_freq;

float realVoltage, realCurrent, realPower;

#define BL0942_BAUD_RATE 4800

#define BL0942_READ_COMMAND 0x58

int BL0942_TryToGetNextBL0942Packet() {
	int cs;
	int i;
	int c_garbage_consumed = 0;
	byte a;
	float newPower, newCurrent, newVoltage;
	byte checksum;
	int BL0942_PACKET_LEN = 23;

	cs = UART_GetDataSize();


	if(cs < BL0942_PACKET_LEN) {
		return 0;
	}
	// skip garbage data (should not happen)
	while(cs > 0) {
		a = UART_GetNextByte(0);
		if(a != 0x55) {
			UART_ConsumeBytes(1);
			c_garbage_consumed++;
			cs--;
		} else {
			break;
		}
	}
	if(c_garbage_consumed > 0){
		addLogAdv(LOG_INFO, LOG_FEATURE_BL0942,"Consumed %i unwanted non-header byte in BL0942 buffer\n", c_garbage_consumed);
	}
	if(cs < BL0942_PACKET_LEN) {
		return 0;
	}
	a = UART_GetNextByte(0);
	if(a != 0x55) {
		return 0;
	}
	checksum = BL0942_READ_COMMAND;

	for(i = 0; i < BL0942_PACKET_LEN-1; i++) {
		checksum += UART_GetNextByte(i);
	}
	checksum ^= 0xFF;

#if 0
	{
		char buffer_for_log[128];
		char buffer2[32];
		buffer_for_log[0] = 0;
		for(i = 0; i < BL0942_PACKET_LEN; i++) {
			sprintf(buffer2,"%02X ",UART_GetNextByte(i));
			strcat_safe(buffer_for_log,buffer2,sizeof(buffer_for_log));
		}
		addLogAdv(LOG_INFO, LOG_FEATURE_BL0942,"BL0942 received: %s\n", buffer_for_log);
	}
#endif
	if(checksum != UART_GetNextByte(BL0942_PACKET_LEN-1)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_BL0942,"Skipping packet with bad checksum %02X wanted %02X\n",checksum,UART_GetNextByte(BL0942_PACKET_LEN-1));
		UART_ConsumeBytes(BL0942_PACKET_LEN);
		return 1;
	}
	//startDriver BL0942
	raw_unscaled_current = (UART_GetNextByte(3) << 16) | (UART_GetNextByte(2) << 8) | UART_GetNextByte(1);  
	raw_unscaled_voltage = (UART_GetNextByte(6) << 16) | (UART_GetNextByte(5) << 8) | UART_GetNextByte(4);  
	raw_unscaled_power = (UART_GetNextByte(12) << 24) | (UART_GetNextByte(11) << 16) | (UART_GetNextByte(10) << 8);  
	raw_unscaled_power = (raw_unscaled_power >> 8);

	raw_unscaled_freq = (UART_GetNextByte(17) << 8) | UART_GetNextByte(16);  

	// those are not values like 230V, but unscaled
	//addLogAdv(LOG_INFO, LOG_FEATURE_BL0942,"Unscaled current %d, voltage %d, power %d, freq %d\n", raw_unscaled_current, raw_unscaled_voltage,raw_unscaled_power,raw_unscaled_freq);

	// those are final values, like 230V
	newPower = (raw_unscaled_power / BL0942_PREF);
	if(newPower!=realPower){
		realPower = newPower;
		MQTT_PublishMain_StringFloat("power",newPower);
	}
	newVoltage = (raw_unscaled_voltage / BL0942_UREF);
	if(newVoltage!=realVoltage){
		realVoltage = newVoltage;
		MQTT_PublishMain_StringFloat("voltage",newVoltage);
	}
	newCurrent = (raw_unscaled_current / BL0942_IREF);
	if(newCurrent!=realCurrent){
		realCurrent = newCurrent;
		MQTT_PublishMain_StringFloat("current",realCurrent);
	}

	{
		char res[128];
		// V=245.107925,I=109.921143,P=0.035618
		sprintf(res,"V=%f,I=%f,P=%f\n",realVoltage,realCurrent,realPower);
		addLogAdv(LOG_INFO, LOG_FEATURE_BL0942,res );

	}

	UART_ConsumeBytes(BL0942_PACKET_LEN);
	
	return BL0942_PACKET_LEN;
}

void BL0942_SendRequest() {
	UART_InitUART(BL0942_BAUD_RATE);
	UART_SendByte(BL0942_READ_COMMAND);
	UART_SendByte(0xAA);
}
int BL0942_PowerSet(const void *context, const char *cmd, const char *args) {
	float realPower;

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_BL0942,"This command needs one argument");
		return 1;
	}
	realPower = atof(args);
	BL0942_PREF = raw_unscaled_power / realPower;
	{
		char dbg[128];
		sprintf(dbg,"CurrentSet: you gave %f, set ref to %f\n", realPower, BL0942_PREF);
		addLogAdv(LOG_INFO, LOG_FEATURE_BL0942,dbg);
	}
	return 0;
}
int BL0942_PowerRef(const void *context, const char *cmd, const char *args) {

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_BL0942,"This command needs one argument");
		return 1;
	}
	BL0942_PREF = atof(args);
	return 0;
}
int BL0942_CurrentRef(const void *context, const char *cmd, const char *args) {

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_BL0942,"This command needs one argument");
		return 1;
	}
	BL0942_IREF = atof(args);
	return 0;
}
int BL0942_VoltageRef(const void *context, const char *cmd, const char *args) {

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_BL0942,"This command needs one argument");
		return 1;
	}
	BL0942_UREF = atof(args);
	return 0;
}
int BL0942_VoltageSet(const void *context, const char *cmd, const char *args) {
	float realV;

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_BL0942,"This command needs one argument");
		return 1;
	}
	realV = atof(args);
	BL0942_UREF = raw_unscaled_voltage / realV;
	{
		char dbg[128];
		sprintf(dbg,"CurrentSet: you gave %f, set ref to %f\n", realV, BL0942_UREF);
		addLogAdv(LOG_INFO, LOG_FEATURE_BL0942,dbg);
	}

	return 0;
}
int BL0942_CurrentSet(const void *context, const char *cmd, const char *args) {
	float realI;

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_BL0942,"This command needs one argument");
		return 1;
	}
	realI = atof(args);
	BL0942_IREF = raw_unscaled_current / realI;
	{
		char dbg[128];
		sprintf(dbg,"CurrentSet: you gave %f, set ref to %f\n", realI, BL0942_IREF);
		addLogAdv(LOG_INFO, LOG_FEATURE_BL0942,dbg);
	}
	return 0;
}
void BL0942_Init() {

	UART_InitUART(BL0942_BAUD_RATE);
	UART_InitReceiveRingBuffer(256);
	CMD_RegisterCommand("PowerSet","",BL0942_PowerSet, "Sets current power value for calibration", NULL);
	CMD_RegisterCommand("VoltageSet","",BL0942_VoltageSet, "Sets current V value for calibration", NULL);
	CMD_RegisterCommand("CurrentSet","",BL0942_CurrentSet, "Sets current I value for calibration", NULL);
	CMD_RegisterCommand("PREF","",BL0942_PowerRef, "Sets the calibration multiplier", NULL);
	CMD_RegisterCommand("VREF","",BL0942_VoltageRef, "Sets the calibration multiplier", NULL);
	CMD_RegisterCommand("IREF","",BL0942_CurrentRef, "Sets the calibration multiplier", NULL);
}
void BL0942_RunFrame() {
	int len;

	addLogAdv(LOG_INFO, LOG_FEATURE_BL0942,"UART buffer size %i\n", UART_GetDataSize());

	len = BL0942_TryToGetNextBL0942Packet();
	if(len > 0) {

	} else {
		BL0942_SendRequest();
	}
}
void BL0942_AppendInformationToHTTPIndexPage(http_request_t *request) {
	char tmp[128];
	sprintf(tmp, "<h2>BL0942 Voltage=%f, Current=%f, Power=%f</h2>",realVoltage,realCurrent, realPower);
    hprintf128(request,tmp);

}



