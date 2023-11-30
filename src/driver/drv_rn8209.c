

#include "../logging/logging.h"
#include "../new_pins.h"
#include "drv_bl_shared.h"
#include "drv_uart.h"

#define BL0942_UART_PACKET_LEN 12
#define BL0942_UART_PACKET_HEAD 12


static void UART_WriteReg(byte reg, byte *data, int len) {
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
static void UART_ReadReg(byte reg) {
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
	UART_ReadReg(0x24);

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
