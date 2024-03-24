
// NOTE:
// Use this for reference:
// https://github.com/RN8209C/RN8209C-SDK/blob/master/src/rn8209c_u.c
// Search for rn8209c_read_current
#include "../logging/logging.h"
#include "../new_pins.h"
#include "drv_bl_shared.h"
#include "drv_uart.h"
#include <math.h>
#include "drv_pwrCal.h"

#define RN8209_RMS_CURRENT_A				0x22
#define RN8209_RMS_CURRENT_A_SIZE			3
#define RN8209_RMS_CURRENT_B				0x23
#define RN8209_RMS_CURRENT_B_SIZE			3
#define RN8209_RMS_VOLTAGE					0x24
#define RN8209_RMS_VOLTAGE_SIZE				3
#define RN8209_RMS_FREQUENCY				0x25
#define RN8209_RMS_FREQUENCY_SIZE			2
#define RN8209_AVG_ACTIVEPOWER_A			0x26
#define RN8209_AVG_ACTIVEPOWER_A_SIZE		4
#define RN8209_AVG_ACTIVEPOWER_B			0x27
#define RN8209_AVG_ACTIVEPOWER_B_SIZE		4
#define RN8209_AVG_REACTIVEPOWER			0x28
#define RN8209_AVG_REACTIVEPOWER_SIZE		4
#define RN8209_AVG_REACTIVEENERGY_1			0x29
#define RN8209_AVG_REACTIVEENERGY_1_SIZE	3
#define RN8209_AVG_REACTIVEENERGY_2			0x2A
#define RN8209_AVG_REACTIVEENERGY_2_SIZE	3

static void RN8209_WriteReg(byte reg, byte *data, int len) {
	byte crc;

	reg = reg | 0x80;

    crc = reg;

	UART_SendByte(reg);
    for (int i = 0; i < len; i++) {
        UART_SendByte(data[i]);
        crc += data[i];
    }
	crc = ~crc;
	
    UART_SendByte(crc);
}
static bool RN8029_ReadReg(byte reg, unsigned int *res) {
	byte crc;
	byte data[32];
	int size, i;
	int dataSize;

	UART_SendByte(reg);

	rtos_delay_milliseconds(30);

	data[0] = reg;
	size = 1;
	crc = reg;
	while(UART_GetDataSize()) {
		data[size] = UART_GetByte(0);
		ADDLOG_WARN(LOG_FEATURE_ENERGYMETER,
			"UA %i\n",
			(int)data[size]);
		size++;
		UART_ConsumeBytes(1);
	}
	for (i = 1; i < size - 1; i++) {
		crc += data[i];
	}
	crc = ~crc;
	if (crc != data[size - 1]) {
		ADDLOG_WARN(LOG_FEATURE_ENERGYMETER,
			"CRC BAD, expected %i, got %i\n", (int)crc, (int)data[size - 1]);
		return 1;
	}
	ADDLOG_WARN(LOG_FEATURE_ENERGYMETER,
		"CRC OK\n");
	*res = 0;
	dataSize = size - 2;
	// Multi-byte registers will first transmit high-byte contents followed by low-byte contents
	for (i = 0; i < dataSize; i++) {
		int val = data[i + 1];
		*res += val << ((dataSize-1-i) * 8);
	}
	return 0;
}
#define DEFAULT_VOLTAGE_CAL 8810
#define DEFAULT_CURRENT_CAL 57
#define DEFAULT_POWER_CAL 71572720
// startDriver RN8209
void RN8209_Init(void) {
	UART_InitUART(4800, 1);
	UART_InitReceiveRingBuffer(256);

	BL_Shared_Init();
	PwrCal_Init(PWR_CAL_DIVIDE, DEFAULT_VOLTAGE_CAL, DEFAULT_CURRENT_CAL,
		DEFAULT_POWER_CAL);
}
unsigned int g_voltage = 0, g_currentA = 0, g_currentB = 0, g_powerA = 0, g_powerB = 0;
int g_meas = 0;
void RN8029_RunEverySecond(void) {
	float final_v;
	float final_c;
	float final_p;

	g_meas++;
	g_meas %= 5;
	switch (g_meas) {
	case 0:
		RN8029_ReadReg(RN8209_RMS_VOLTAGE, &g_voltage);
		break;
	case 1:
		if (RN8029_ReadReg(RN8209_RMS_CURRENT_A, &g_currentA) == 0) {
			if (g_currentA & 0x800000) {
				g_currentA = 0;
			}
		}
		break;
	case 2:
		RN8029_ReadReg(RN8209_RMS_CURRENT_B, &g_currentB);
		break;
	case 3:
		if (RN8029_ReadReg(RN8209_AVG_ACTIVEPOWER_A, &g_powerA) == 0) {
			if (g_powerA & 0x80000000) {
				g_powerA = ~g_powerA;
				g_powerA += 1;
			}
		}
		break;
	case 4:
		RN8029_ReadReg(RN8209_AVG_ACTIVEPOWER_B, &g_powerB);
		break;
	}

	ADDLOG_WARN(LOG_FEATURE_ENERGYMETER,
		"V %u, C %u %u, P %u %u\n", g_voltage, g_currentA, g_currentB, g_powerA, g_powerB);

	PwrCal_Scale(g_voltage, g_currentA, g_powerA, &final_v, &final_c, &final_p);
	BL_ProcessUpdate(final_v, final_c, final_p, NAN, NAN);
}
/*
Send: 36 (command code)

Received:
Warn:EnergyMeter:UA 32
Warn:EnergyMeter:UA 51
Warn:EnergyMeter:UA 225
Warn:EnergyMeter:UA 167

Let's verify checksum (sum modulo 256 and negated):
(36+32+51+225)%256=88
88  -> 01011000
167 -> 10100111

Send: 36 (command code)
Warn:EnergyMeter:UA 32
Warn:EnergyMeter:UA 38
Warn:EnergyMeter:UA 227
Warn:EnergyMeter:UA 178
Let's verify checksum (sum modulo 256 and negated):
(36+32+38+227)%256=77
77  -> 01001101
178 -> 10110010
*/
