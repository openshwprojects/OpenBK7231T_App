

#include "../logging/logging.h"
#include "../new_pins.h"
#include "drv_bl_shared.h"
#include "drv_uart.h"

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
static void RN8029_ReadReg(byte reg) {
	byte crc;
	byte data[32];
	int size, i;

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
	if (crc == data[size - 1]) {
		ADDLOG_WARN(LOG_FEATURE_ENERGYMETER,
			"CRC OK\n");
	}
	else {
		ADDLOG_WARN(LOG_FEATURE_ENERGYMETER,
			"CRC BAD, expected %i, got %i\n",(int)crc,(int)data[size - 1]);
	}
}
// startDriver RN8209
void RN8209_Init(void) {
	UART_InitUART(4800, 1);
	UART_InitReceiveRingBuffer(256);

}

void RN8029_RunEverySecond(void) {
	RN8029_ReadReg(0x24);

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
