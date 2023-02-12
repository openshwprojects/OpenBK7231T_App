#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_public.h"
#include "drv_local.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"
#include <math.h>

static float BL0942_PREF = 598;
static float BL0942_UREF = 15188;
static float BL0942_IREF = 251210;

static int raw_unscaled_voltage;
static int raw_unscaled_current;
static int raw_unscaled_power;
static int raw_unscaled_freq;

static float valid_voltage = 0.0f;
static float valid_current = 0.0f;
static float valid_power = 0.0f;

#define BL0942_BAUD_RATE 4800

#define BL0942_READ_COMMAND 0x58


int BL0942_TryToGetNextBL0942Packet() {
	int cs;
	int i;
	int c_garbage_consumed = 0;
	byte a;
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
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"Consumed %i unwanted non-header byte in BL0942 buffer\n", c_garbage_consumed);
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

#if 1
    {
		char buffer_for_log[128];
		char buffer2[32];
		buffer_for_log[0] = 0;
		for(i = 0; i < BL0942_PACKET_LEN; i++) {
			snprintf(buffer2, sizeof(buffer2), "%02X ",UART_GetNextByte(i));
			strcat_safe(buffer_for_log,buffer2,sizeof(buffer_for_log));
		}
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"BL0942 received: %s\n", buffer_for_log);
	}
#endif
	if(checksum != UART_GetNextByte(BL0942_PACKET_LEN-1)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"Skipping packet with bad checksum %02X wanted %02X\n",checksum,UART_GetNextByte(BL0942_PACKET_LEN-1));
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
	addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"Unscaled current %d, voltage %d, power %d, freq %d\n", raw_unscaled_current, raw_unscaled_voltage,raw_unscaled_power,raw_unscaled_freq);
    addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"HEX Current: %08lX; Voltage: %08lX; Power: %08lX;\n", (unsigned long)raw_unscaled_current, 
              (unsigned long)raw_unscaled_voltage, (unsigned long)raw_unscaled_power);

	// those are final values, like 230V
	{
		float power, voltage, current;
		power = (raw_unscaled_power / BL0942_PREF);
		voltage = (raw_unscaled_voltage / BL0942_UREF);
		current = (raw_unscaled_current / BL0942_IREF);
        addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"Real current %1.3lf, voltage %1.1lf, power %1.1lf\n", current, voltage, power);

        /* Logical check of values */
        if (abs(power) <= (voltage * current * 1.1f))
        {
            valid_voltage = voltage;
            valid_current = current;
            valid_power = power;
            addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"Valid current %1.3lf, voltage %1.1lf, power %1.1lf\n", valid_current, valid_voltage, valid_power);
        } else {
            addLogAdv(LOG_WARN, LOG_FEATURE_ENERGYMETER,"Invalid Power value: %1.3lf Expected: %1.3lf\n", abs(power), (voltage * current));
        }

		BL_ProcessUpdate(valid_voltage, valid_current, valid_power);
	}


#if 0
	{
		char res[128];
		// V=245.107925,I=109.921143,P=0.035618
		snprintf(res, sizeof(res),"V=%f,I=%f,P=%f\n",lastReadings[OBK_VOLTAGE],lastReadings[OBK_CURRENT],lastReadings[OBK_POWER]);
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,res );
	}
#endif

	UART_ConsumeBytes(BL0942_PACKET_LEN);

	return BL0942_PACKET_LEN;
}

void BL0942_SendRequest() {
	UART_InitUART(BL0942_BAUD_RATE);
	UART_SendByte(BL0942_READ_COMMAND);
	UART_SendByte(0xAA);
}
commandResult_t BL0942_PowerSet(const void *context, const char *cmd, const char *args, int cmdFlags) {
	float realPower;

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"This command needs one argument");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	realPower = atof(args);
	BL0942_PREF = raw_unscaled_power / realPower;

	// UPDATE: now they are automatically saved
	CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_POWER,BL0942_PREF);

	{
		char dbg[128];
		snprintf(dbg, sizeof(dbg),"PowerSet: you gave %f, set ref to %f\n", realPower, BL0942_PREF);
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,dbg);
	}
	return CMD_RES_OK;
}
commandResult_t BL0942_PowerRef(const void *context, const char *cmd, const char *args, int cmdFlags) {

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"This command needs one argument");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	BL0942_PREF = atof(args);

	// UPDATE: now they are automatically saved
	CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_POWER,BL0942_PREF);

	return CMD_RES_OK;
}
commandResult_t BL0942_CurrentRef(const void *context, const char *cmd, const char *args, int cmdFlags) {

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"This command needs one argument");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	BL0942_IREF = atof(args);

	// UPDATE: now they are automatically saved
	CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_CURRENT,BL0942_IREF);

	return CMD_RES_OK;
}
commandResult_t BL0942_VoltageRef(const void *context, const char *cmd, const char *args, int cmdFlags) {

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"This command needs one argument");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	BL0942_UREF = atof(args);

	// UPDATE: now they are automatically saved
	CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_VOLTAGE,BL0942_UREF);

	return CMD_RES_OK;
}
commandResult_t BL0942_VoltageSet(const void *context, const char *cmd, const char *args, int cmdFlags) {
	float realV;

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"This command needs one argument");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	realV = atof(args);
	BL0942_UREF = raw_unscaled_voltage / realV;

	// UPDATE: now they are automatically saved
	CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_VOLTAGE,BL0942_UREF);

	{
		char dbg[128];
		snprintf(dbg, sizeof(dbg),"VoltageSet: you gave %f, set ref to %f\n", realV, BL0942_UREF);
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,dbg);
	}

	return CMD_RES_OK;
}
commandResult_t BL0942_CurrentSet(const void *context, const char *cmd, const char *args, int cmdFlags) {
	float realI;

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"This command needs one argument");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	realI = atof(args);
	BL0942_IREF = raw_unscaled_current / realI;

	// UPDATE: now they are automatically saved
	CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_CURRENT,BL0942_IREF);

	{
		char dbg[128];
		snprintf(dbg, sizeof(dbg),"CurrentSet: you gave %f, set ref to %f\n", realI, BL0942_IREF);
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,dbg);
	}
	return CMD_RES_OK;
}
void BL0942_Init() 
{
    BL_Shared_Init();

	// UPDATE: now they are automatically saved
	BL0942_UREF = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_VOLTAGE,BL0942_UREF);
	BL0942_PREF = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_POWER,BL0942_PREF);
	BL0942_IREF = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_CURRENT,BL0942_IREF);    

	UART_InitUART(BL0942_BAUD_RATE);
	UART_InitReceiveRingBuffer(256);
	CMD_RegisterCommand("PowerSet",BL0942_PowerSet, NULL);
	CMD_RegisterCommand("VoltageSet",BL0942_VoltageSet, NULL);
	CMD_RegisterCommand("CurrentSet",BL0942_CurrentSet, NULL);
	CMD_RegisterCommand("PREF",BL0942_PowerRef, NULL);
	CMD_RegisterCommand("VREF",BL0942_VoltageRef, NULL);
	CMD_RegisterCommand("IREF",BL0942_CurrentRef, NULL);
}
void BL0942_RunFrame() {
	int len;

	//addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"UART buffer size %i\n", UART_GetDataSize());

	len = BL0942_TryToGetNextBL0942Packet();
	if(len > 0) {

	} else {
		BL0942_SendRequest();
	}
}

