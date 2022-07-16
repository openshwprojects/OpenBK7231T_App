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

static float CSE7766_PREF = 3150.261719;
static float CSE7766_UREF = 6.300000;
static float CSE7766_IREF = 251210;

static int raw_unscaled_voltage;
static int raw_unscaled_current;
static int raw_unscaled_power;
//static int raw_unscaled_freq;


#define CSE7766_BAUD_RATE 4800


// startDriver CSE7766
int CSE7766_TryToGetNextCSE7766Packet() {
	int cs;
	int i;
	int c_garbage_consumed = 0;
	byte a;
	byte checksum;
	int CSE7766_PACKET_LEN = 24;
	byte header;

	cs = UART_GetDataSize();


	if(cs < CSE7766_PACKET_LEN) {
		return 0;
	}
	header = UART_GetNextByte(0);
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
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"Consumed %i unwanted non-header byte in CSE7766 buffer\n", c_garbage_consumed);
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
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"CSE7766 received: %s\n", buffer_for_log);
	}
#endif
	if(checksum != UART_GetNextByte(CSE7766_PACKET_LEN-1)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"Skipping packet with bad checksum %02X wanted %02X\n",checksum,UART_GetNextByte(CSE7766_PACKET_LEN-1));
		UART_ConsumeBytes(CSE7766_PACKET_LEN);
		return 1;
	}
	//addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"CSE checksum ok");

	{
		unsigned char adjustement;
		//long power_cycle_first = 0;
		long cf_pulses = 0;


		// samples captured by me on 07 07 2022
		// 0   1  2  3  4  5  6  7  8 9  10 11 12 13 14 15 16 17 18 19 20 21 22 23
		// H  Id VCal---- Voltage- ICal---- Current- PCal---- Power--- Ad CF--- Ck
		// F2 5A 02 D5 00 00 05 A7 00 3C 05 03 2B F3 4D B2 A0 9C 98 CA 61 24 90 97 
		// F2 5A 02 D5 00 00 05 A7 00 3C 05 03 77 4B 4D B2 A0 AA FE 56 61 24 90 3B 		// F2 5A 02 D5 00 00 05 AB 00 3C 05 03 77 4B 4D B2 A0 BA FC 48 61 24 90 3F 		// F2 5A 02 D5 00 00 05 AB 00 3C 05 05 38 DB 4D B2 A0 C9 60 D5 61 24 90 92 

		// samples with disabled relay (?not sure, doing it remotely)
		// power is 0, current still non-zero
		// power should be 54.5W
		/*
		0   1  2  3  4  5  6  7  8 9  10 11 12 13 14 15 16 17 18 19 20 21 22 23
		H  Id VCal---- Voltage- ICal---- Current- PCal---- Power--- Ad CF--- Ck
		F2 5A 02 D5 00 00 05 B7 00 3C 05 03 5D C5 4D B2 A0 A7 BB 9C 61 2A 61 82 		F2 5A 02 D5 00 00 05 B7 00 3C 05 03 5D C5 4D B2 A0 B6 20 29 61 2A 61 83 		F2 5A 02 D5 00 00 F2 5A 02 D5 00 00 05 B7 00 3C 05 03 5D C5 4D B2 A0 C6 
		F2 5A 02 D5 00 00 05 B7 00 3C 05 03 5D C5 4D B2 A0 D4 82 A8 61 2A 61 82 
		F2 5A 02 D5 00 00 05 B7 00 3C 05 03 76 DF 4D B2 A0 E4 7F 9B 61 2A E8 B5 

		*/
		// samples with enabled relay (current, power, voltag enon-zero)
		/*
		0   1  2  3  4  5  6  7  8 9  10 11 12 13 14 15 16 17 18 19 20 21 22 23
		H  Id VCal---- Voltage- ICal---- Current- PCal---- Power--- Ad CF--- Ck		55 5A 02 D5 00 00 05 A9 00 3C 38 00 FD 5C 4D B2 A0 02 95 7C 71 48 23 AD 		55 5A 02 D5 00 00 05 A9 00 3C 05 00 FD 73 4D B2 A0 02 97 27 71 48 28 76 		55 5A 02 D5 00 00 05 A7 00 3C 05 00 FD 73 4D B2 A0 02 96 1F 71 48 2E 71 		55 5A 02 D5 00 00 05 A7 00 3C 05 00 FD 73 4D B2 A0 02 97 5C 71 48 34 B5 		55 5A 02 D5 00 00 05 A7 00 3C 05 00 FD 9C 4D B2 A0 02 96 80 71 48 3A 07 		55 5A 02 D5 00 00 05 A6 00 3C 05 00 FD 9C 4D B2 A0 02 93 C2 71 48 40 4B 		55 5A 02 D5 00 00 05 A6 00 3C 05 00 FD 9C 4D B2 A0 02 96 54 71 48 46 E6 		55 5A 02 D5 00 00 05 A6 00 3C 05 00 FD AB 4D B2 A0 02 96 5F 71 48 4B 05 
		*/
		//
			
		
		

		adjustement = UART_GetNextByte(20);
		raw_unscaled_voltage = UART_GetNextByte(5) << 16 | UART_GetNextByte(6) << 8 | UART_GetNextByte(7);
		raw_unscaled_current = UART_GetNextByte(11) << 16 | UART_GetNextByte(12) << 8 | UART_GetNextByte(13);
		raw_unscaled_power = UART_GetNextByte(17) << 16 | UART_GetNextByte(18) << 8 | UART_GetNextByte(19);
		cf_pulses = UART_GetNextByte(21) << 8 | UART_GetNextByte(22);

		// i am not sure about these flags
		if (adjustement & 0x40) {  // Voltage valid
		
		} else {
			raw_unscaled_voltage = 0;
		}
		if (adjustement & 0x10) {  // Power valid
			if ((header & 0xF2) == 0xF2) {  // Power cycle exceeds range
				//power_cycle = 0;
			} else {

			}
		} else {
			raw_unscaled_power = 0;
		}
		if (adjustement & 0x20) {  // Current valid

		} else {
			raw_unscaled_current = 0;
		}

		// those are final values, like 230V
		{
			float power, voltage, current;
			power = CSE7766_PREF / raw_unscaled_power;
			voltage = CSE7766_UREF / raw_unscaled_voltage;
			current = CSE7766_IREF / raw_unscaled_current;

			BL_ProcessUpdate(voltage,current,power);
		}
	}




#if 0
	{
		char res[128];
		// V=245.107925,I=109.921143,P=0.035618
		sprintf(res,"V=%f,I=%f,P=%f\n",lastReadings[OBK_VOLTAGE],lastReadings[OBK_CURRENT],lastReadings[OBK_POWER]);
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,res );
	}
#endif

	UART_ConsumeBytes(CSE7766_PACKET_LEN);

	return CSE7766_PACKET_LEN;
}

int CSE7766_PowerSet(const void *context, const char *cmd, const char *args, int cmdFlags) {
	float realPower;

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"This command needs one argument");
		return 1;
	}
	realPower = atof(args);
	CSE7766_PREF = realPower / raw_unscaled_power;
	{
		char dbg[128];
		sprintf(dbg,"PowerSet: you gave %f, set ref to %f\n", realPower, CSE7766_PREF);
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,dbg);
	}
	return 0;
}
int CSE7766_PowerRef(const void *context, const char *cmd, const char *args, int cmdFlags) {

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"This command needs one argument");
		return 1;
	}
	CSE7766_PREF = atof(args);
	return 0;
}
int CSE7766_CurrentRef(const void *context, const char *cmd, const char *args, int cmdFlags) {

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"This command needs one argument");
		return 1;
	}
	CSE7766_IREF = atof(args);
	return 0;
}
int CSE7766_VoltageRef(const void *context, const char *cmd, const char *args, int cmdFlags) {

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"This command needs one argument");
		return 1;
	}
	CSE7766_UREF = atof(args);
	return 0;
}
int CSE7766_VoltageSet(const void *context, const char *cmd, const char *args, int cmdFlags) {
	float realV;

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"This command needs one argument");
		return 1;
	}
	realV = atof(args);
	CSE7766_UREF = realV / raw_unscaled_voltage;
	{
		char dbg[128];
		sprintf(dbg,"CurrentSet: you gave %f, set ref to %f\n", realV, CSE7766_UREF);
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,dbg);
	}

	return 0;
}
int CSE7766_CurrentSet(const void *context, const char *cmd, const char *args, int cmdFlags) {
	float realI;

	if(args==0||*args==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"This command needs one argument");
		return 1;
	}
	realI = atof(args);
	CSE7766_IREF = realI / raw_unscaled_current;
	{
		char dbg[128];
		sprintf(dbg,"CurrentSet: you gave %f, set ref to %f\n", realI, CSE7766_IREF);
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,dbg);
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

	//addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"UART buffer size %i\n", UART_GetDataSize());

	CSE7766_TryToGetNextCSE7766Packet();
}

