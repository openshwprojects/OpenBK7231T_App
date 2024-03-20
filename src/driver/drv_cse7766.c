// NOTE: this is the same as HLW8032
#include "drv_cse7766.h"

#include <math.h>

#include "../logging/logging.h"
#include "../new_pins.h"
#include "drv_bl_shared.h"
#include "drv_pwrCal.h"
#include "drv_uart.h"

#define DEFAULT_VOLTAGE_CAL 1.94034719
#define DEFAULT_CURRENT_CAL 251210
#define DEFAULT_POWER_CAL 1.88214409

#define CSE7766_BAUD_RATE 4800

int CSE7766_TryToGetNextCSE7766Packet() {
	int cs;
	int i;
	int c_garbage_consumed = 0;
	//byte a;
	byte checksum;
	int CSE7766_PACKET_LEN = 24;
	byte header;

	cs = UART_GetDataSize();


	if(cs < CSE7766_PACKET_LEN) {
		return 0;
	}
    header = UART_GetByte(0);
    // skip garbage data (should not happen)
	while(cs > 0) {
        //a = UART_GetByte(1);
        if(UART_GetByte(0) != 0x55 || UART_GetByte(1) != 0x5A) {
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
    //a = UART_GetByte(1);
    if(UART_GetByte(0) != 0x55 || UART_GetByte(1) != 0x5A) {
		return 0;
	}
	checksum = 0;

	for(i = 2; i < CSE7766_PACKET_LEN-1; i++) {
        checksum += UART_GetByte(i);
    }

#if 1
	{
		char buffer_for_log[128];
		char buffer2[32];
		buffer_for_log[0] = 0;
		for(i = 0; i < CSE7766_PACKET_LEN; i++) {
            snprintf(buffer2, sizeof(buffer2), "%02X ", UART_GetByte(i));
            strcat_safe(buffer_for_log,buffer2,sizeof(buffer_for_log));
		}
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"CSE7766 received: %s\n", buffer_for_log);
	}
#endif
	if(checksum != UART_GetByte(CSE7766_PACKET_LEN-1)) {
        ADDLOG_INFO(LOG_FEATURE_ENERGYMETER,
                    "Skipping packet with bad checksum %02X wanted %02X\n",
                    checksum, UART_GetByte(CSE7766_PACKET_LEN - 1));
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
		// F2 5A 02 D5 00 00 05 A7 00 3C 05 03 77 4B 4D B2 A0 AA FE 56 61 24 90 3B 
		// F2 5A 02 D5 00 00 05 AB 00 3C 05 03 77 4B 4D B2 A0 BA FC 48 61 24 90 3F 
		// F2 5A 02 D5 00 00 05 AB 00 3C 05 05 38 DB 4D B2 A0 C9 60 D5 61 24 90 92 

		// samples with disabled relay (?not sure, doing it remotely)
		// power is 0, current still non-zero
		// power should be 54.5W
		/*
		0   1  2  3  4  5  6  7  8 9  10 11 12 13 14 15 16 17 18 19 20 21 22 23
		H  Id VCal---- Voltage- ICal---- Current- PCal---- Power--- Ad CF--- Ck
		F2 5A 02 D5 00 00 05 B7 00 3C 05 03 5D C5 4D B2 A0 A7 BB 9C 61 2A 61 82 
		F2 5A 02 D5 00 00 05 B7 00 3C 05 03 5D C5 4D B2 A0 B6 20 29 61 2A 61 83 
		F2 5A 02 D5 00 00 F2 5A 02 D5 00 00 05 B7 00 3C 05 03 5D C5 4D B2 A0 C6 
		F2 5A 02 D5 00 00 05 B7 00 3C 05 03 5D C5 4D B2 A0 D4 82 A8 61 2A 61 82 
		F2 5A 02 D5 00 00 05 B7 00 3C 05 03 76 DF 4D B2 A0 E4 7F 9B 61 2A E8 B5 

		*/
		// samples with enabled relay (current, power, voltag enon-zero)
		/*
		0   1  2  3  4  5  6  7  8 9  10 11 12 13 14 15 16 17 18 19 20 21 22 23
		H  Id VCal---- Voltage- ICal---- Current- PCal---- Power--- Ad CF--- Ck
		55 5A 02 D5 00 00 05 A9 00 3C 38 00 FD 5C 4D B2 A0 02 95 7C 71 48 23 AD 
		55 5A 02 D5 00 00 05 A9 00 3C 05 00 FD 73 4D B2 A0 02 97 27 71 48 28 76 
		55 5A 02 D5 00 00 05 A7 00 3C 05 00 FD 73 4D B2 A0 02 96 1F 71 48 2E 71 
		55 5A 02 D5 00 00 05 A7 00 3C 05 00 FD 73 4D B2 A0 02 97 5C 71 48 34 B5 
		55 5A 02 D5 00 00 05 A7 00 3C 05 00 FD 9C 4D B2 A0 02 96 80 71 48 3A 07 
		55 5A 02 D5 00 00 05 A6 00 3C 05 00 FD 9C 4D B2 A0 02 93 C2 71 48 40 4B 
		55 5A 02 D5 00 00 05 A6 00 3C 05 00 FD 9C 4D B2 A0 02 96 54 71 48 46 E6 
		55 5A 02 D5 00 00 05 A6 00 3C 05 00 FD AB 4D B2 A0 02 96 5F 71 48 4B 05 
		*/
		//
		// 70W 240V sample from Elektroda user
		/*
		H  Id VCal---- Voltage- ICal---- Current- PCal---- Power--- Ad CF--- Ck
		55 5A 02 FC D8 00 06 28 00 41 32 00 C9 FE 53 7B 18 02 3B D4 71 71 E3 FA
		55 5A 02 FC D8 00 06 28 00 41 32 00 C9 FE 53 7B 18 02 3B D4 71 71 E3 FA
		55 5A 02 FC D8 00 06 28 00 41 32 00 C9 FE 53 7B 18 02 3C 28 71 71 EA 56
		55 5A 02 FC D8 00 06 28 00 41 32 00 D7 F2 53 7B 18 02 3B 71 71 71 F1 A7
		55 5A 02 FC D8 00 06 2F 00 41 32 00 D7 F2 53 7B 18 02 3D AF 71 71 F8 F5
		55 5A 02 FC D8 00 06 2F 00 41 32 00 D7 F2 53 7B 18 02 3E 9F 71 71 FE EC

		backlog startDriver CSE7766; uartFakeHex 555A02FCD800062F00413200D7F2537B18023E9F7171FEEC
		*/

#define CSC_GetByte(x) ((unsigned long)UART_GetByte(x))

        adjustement = UART_GetByte(20);
        int vol_par =
			CSC_GetByte(2) << 16 | CSC_GetByte(3) << 8 | CSC_GetByte(4);
        int cur_par =
			CSC_GetByte(8) << 16 | CSC_GetByte(9) << 8 | CSC_GetByte(10);
        int pow_par =
			CSC_GetByte(14) << 16 | CSC_GetByte(15) << 8 | CSC_GetByte(16);
        float raw_unscaled_voltage =
			CSC_GetByte(5) << 16 | CSC_GetByte(6) << 8 | UART_GetByte(7);
        float raw_unscaled_current =
			CSC_GetByte(11) << 16 | CSC_GetByte(12) << 8 | CSC_GetByte(13);
        float raw_unscaled_power =
			CSC_GetByte(17) << 16 | CSC_GetByte(18) << 8 | CSC_GetByte(19);
        cf_pulses = CSC_GetByte(21) << 8 | CSC_GetByte(22);

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

		if (raw_unscaled_voltage) {
			raw_unscaled_voltage = vol_par / raw_unscaled_voltage;
		}
		if (raw_unscaled_current) {
			raw_unscaled_current = cur_par / raw_unscaled_current;
		}
		if (raw_unscaled_power) {
			raw_unscaled_power = pow_par / raw_unscaled_power;
		}

        // those are final values, like 230V
        float voltage, current, power;
        PwrCal_Scale(raw_unscaled_voltage, raw_unscaled_current,
                     raw_unscaled_power, &voltage, &current, &power);
        BL_ProcessUpdate(voltage, current, power, NAN, NAN);
    }

#if 0
	{
		char res[128];
		// V=245.107925,I=109.921143,P=0.035618
		snprintf(res, sizeof(res), "V=%f,I=%f,P=%f\n",lastReadings[OBK_VOLTAGE],lastReadings[OBK_CURRENT],lastReadings[OBK_POWER]);
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,res );
	}
#endif

	UART_ConsumeBytes(CSE7766_PACKET_LEN);

	return CSE7766_PACKET_LEN;
}

void CSE7766_Init(void) {
    BL_Shared_Init();

    PwrCal_Init(PWR_CAL_MULTIPLY, DEFAULT_VOLTAGE_CAL, DEFAULT_CURRENT_CAL,
                DEFAULT_POWER_CAL);

	UART_InitUART(CSE7766_BAUD_RATE, 0);
	UART_InitReceiveRingBuffer(512);
}

void CSE7766_RunEverySecond(void) {
    //addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"UART buffer size %i\n", UART_GetDataSize());

	CSE7766_TryToGetNextCSE7766Packet();
}
