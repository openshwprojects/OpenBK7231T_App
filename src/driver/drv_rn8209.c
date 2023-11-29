

#include "../logging/logging.h"
#include "../new_pins.h"
#include "drv_bl_shared.h"
#include "drv_uart.h"

#define BL0942_UART_PACKET_LEN 12
#define BL0942_UART_PACKET_HEAD 12

static int UART_TryToGetNextPacket(void) {
	int cs;
	int i;
	int c_garbage_consumed = 0;
	byte checksum;

	do {
		cs = UART_GetDataSize();
		i = UART_GetByte(0);

		ADDLOG_WARN(LOG_FEATURE_ENERGYMETER,
		                   "UA %i\n",
		                i);
		UART_ConsumeBytes(1);
	} while (cs > 0);


	//if(cs < BL0942_UART_PACKET_LEN) {
	//	return 0;
	//}
	//// skip garbage data (should not happen)
	//while(cs > 0) {
 //       if (UART_GetByte(0) != BL0942_UART_PACKET_HEAD) {
	//		UART_ConsumeBytes(1);
	//		c_garbage_consumed++;
	//		cs--;
	//	} else {
	//		break;
	//	}
	//}
	//if(c_garbage_consumed > 0){
 //       ADDLOG_WARN(LOG_FEATURE_ENERGYMETER,
 //                   "Consumed %i unwanted non-header byte in BL0942 buffer\n",
 //                   c_garbage_consumed);
	//}
	//if(cs < BL0942_UART_PACKET_LEN) {
	//	return 0;
	//}
 //   if (UART_GetByte(0) != 0x55)
	//	return 0;
 //   checksum = BL0942_UART_CMD_READ(BL0942_UART_ADDR);

 //   for(i = 0; i < BL0942_UART_PACKET_LEN-1; i++) {
 //       checksum += UART_GetByte(i);
	//}
	//checksum ^= 0xFF;

 //   if (checksum != UART_GetByte(BL0942_UART_PACKET_LEN - 1)) {
 //       ADDLOG_WARN(LOG_FEATURE_ENERGYMETER,
 //                   "Skipping packet with bad checksum %02X wanted %02X\n",
 //                   UART_GetByte(BL0942_UART_PACKET_LEN - 1), checksum);
 //       UART_ConsumeBytes(BL0942_UART_PACKET_LEN);
	//	return 1;
	//}


 //   UART_ConsumeBytes(BL0942_UART_PACKET_LEN);
	return 0;
}

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
static void UART_ReadReg(byte reg, byte *data, int len) {
	byte crc;

	UART_SendByte(reg);

	/*crc = reg;

	for (int i = 0; i < len; i++) {
		UART_SendByte(data[i]);
		crc += data[i];
	}
	crc = ~crc;

	UART_SendByte(crc);*/
}

void RN8209_UART_Init(void) {
	UART_InitUART(4800);
	UART_InitReceiveRingBuffer(256);

}

void RN8029_UART_RunEverySecond(void) {
    UART_TryToGetNextPacket();

	UART_ReadReg(0x24,0,0);

}

