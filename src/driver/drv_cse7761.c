
// based on xnrg_19_cse7761.ino from arendst/Tasmota
// Unfinished port, not working, delayed until uart tests are done
#include "../obk_config.h"

#if ENABLE_DRIVER_CSE7761

#include <math.h>

#include "../logging/logging.h"
#include "../new_pins.h"
#include "drv_bl_shared.h"
#include "drv_pwrCal.h"
#include "drv_uart.h"

#define CSE7761_BAUD_RATE 38400

void Cse7761Write(uint32_t reg, uint32_t data) {
	uint8_t buffer[5];

	buffer[0] = 0xA5;
	buffer[1] = reg;
	uint32_t len = 2;
	if (data) {
		if (data < 0xFF) {
			buffer[2] = data & 0xFF;
			len = 3;
		}
		else {
			buffer[2] = (data >> 8) & 0xFF;
			buffer[3] = data & 0xFF;
			len = 4;
		}
		uint8_t crc = 0;
		for (uint32_t i = 0; i < len; i++) {
			crc += buffer[i];
		}
		buffer[len] = ~crc;
		len++;
	}

	addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "CSE7761 send %i\n", len);
	for (int i = 0; i < len; i++) {
		UART_SendByte(buffer[i]);
	}

}

bool Cse7761ReadOnce(uint32_t log_level, uint32_t reg, uint32_t size, uint32_t* value) {
	while (UART_GetDataSize()) { UART_ConsumeBytes(1); }

	Cse7761Write(reg, 0);

	uint8_t buffer[8] = { 0 };
	uint32_t rcvd = 0;
	//uint32_t timeout = millis() + 6;

	rtos_delay_milliseconds(6);
	for (int i = 0; i < UART_GetDataSize(); i++) {
		if (i < sizeof(buffer)) {
			buffer[i] = UART_GetByte(i);
			rcvd++;
		}
	}
	UART_ConsumeBytes(UART_GetDataSize());
	//while (!TimeReached(timeout) && (rcvd <= size)) {
	//	//  while (!TimeReached(timeout)) {
	//	int value = Cse7761Serial->read();
	//	if ((value > -1) && (rcvd < sizeof(buffer) - 1)) {
	//		buffer[rcvd++] = value;
	//	}
	//}

	addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "CSE7761 rec %i\n", rcvd);
	if (!rcvd) {
		//AddLog(LOG_LEVEL_DEBUG_MORE, PSTR("C61: Rx none"));
		return false;
	}
	//AddLog(LOG_LEVEL_DEBUG_MORE, PSTR("C61: Rx %*_H"), rcvd, buffer);
	if (rcvd > 5) {
		//AddLog(LOG_LEVEL_DEBUG_MORE, PSTR("C61: Rx overflow"));
		return false;
	}

	rcvd--;
	uint32_t result = 0;
	uint8_t crc = 0xA5 + reg;
	for (uint32_t i = 0; i < rcvd; i++) {
		result = (result << 8) | buffer[i];
		crc += buffer[i];
	}
	crc = ~crc;
	if (crc != buffer[rcvd]) {
		//AddLog(log_level, PSTR("C61: Rx %*_H, CRC error %02X"), rcvd + 1, buffer, crc);
		return false;
	}

	*value = result;
	return true;
}


uint32_t Cse7761Read(uint32_t reg, uint32_t size) {
	bool result = false;  // Start loop
	uint32_t retry = 3;   // Retry up to three times
	uint32_t value = 0;   // Default no value
	while (!result && retry) {
		retry--;
		result = Cse7761ReadOnce(0, reg, size, &value);
	}
	return value;
}

uint32_t Cse7761ReadFallback(uint32_t reg, uint32_t prev, uint32_t size) {
	uint32_t value = Cse7761Read(reg, size);
	if (!value) {    // Error so use previous value read
		value = prev;
	}
	return value;
}


int CSE7761_TryToGetNextCSE7761Packet() {
	

	return 0;
}

// startDriver CSE7761
void CSE7761_Init(void) {
    BL_Shared_Init();

   // PwrCal_Init(PWR_CAL_MULTIPLY, DEFAULT_VOLTAGE_CAL, DEFAULT_CURRENT_CAL,
   //             DEFAULT_POWER_CAL);

	// 8 data bits, even parity, 1 stop bit
	UART_InitUART(CSE7761_BAUD_RATE, 1, false);
	UART_InitReceiveRingBuffer(512);
}

void CSE7761_RunEverySecond(void) {
    //addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"UART buffer size %i\n", UART_GetDataSize());

	int syscon = Cse7761Read(0x00, 2);      // Default 0x0A04
   addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"CSE7761 syscon %i\n", syscon);


	CSE7761_TryToGetNextCSE7761Packet();
}

// close ENABLE_DRIVER_CSE7761
#endif

