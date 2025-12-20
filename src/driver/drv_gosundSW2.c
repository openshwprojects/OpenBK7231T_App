// based on info from here:
// https://www.elektroda.com/rtvforum/viewtopic.php?p=21593497#21593497
#include "../obk_config.h"



#if ENABLE_DRIVER_GOSUNDSW2

#include <math.h>
#include <stdint.h>

#include "drv_local.h"
#include "../logging/logging.h"
#include "../new_cfg.h"
#include "../new_pins.h"
#include "../cmnds/cmd_public.h"
#include "drv_uart.h"

#define SW2_BAUDRATE 9600

void DRV_GosundSW2_RunFrame() {
	if (UART_GetDataSize() >= 5) {
		char buff[5];
		for (int i = 0; i < 5; i++) {
			buff[i] = UART_GetByte(i);
		}
		UART_ConsumeBytes(5);
		float dimmerVal = buff[1] / 100.0;

	}

}
void DRV_GosundSW2_Write(float* finalRGBCW) {

	float state = finalRGBCW[0];

	uint8_t dimmerVal = 0;

	if (state > 0.0)
	{
		dimmerVal = (127.0 * state) + 0.5;
		dimmerVal |= 0x80;  /* Bit 7 seems to control the relay */
	}

	addLogAdv(LOG_INFO, LOG_FEATURE_HTTP, "Writing dimmer value %02X", dimmerVal);
	UART_SendByte(dimmerVal);
}
// startDriver GosundSW2
// backlog clearConfig; startDriver GosundSW2
void DRV_GosundSW2_Init() {
	UART_InitUART(SW2_BAUDRATE, 0, false);
	UART_InitReceiveRingBuffer(256);
}


#endif
