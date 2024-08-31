// See: https://www.elektroda.com/rtvforum/viewtopic.php?p=21175431#21175431
// http://www.hmnst.com/a/products/gas/120.html
// http://cn.hmnst.com/uploads/instructions/HGS02.pdf
/*
Used to detect the VOC gas content in the air. The module adopts a highly integrated design and has the characteristics of small size, low power consumption, high sensitivity and fast response speed.



*/
//
#include <math.h>
#include <stdint.h>

#include "../logging/logging.h"
#include "../new_pins.h"
#include "../cmnds/cmd_public.h"
#include "drv_uart.h"

#define HGS02_PACKET_HEADER	0xA0
#define HGS02_UART_PACKET_LEN	16
#define HGS02_RECEIVE_BUFFER_SIZE 128

static int hgs02_baudRate;

float UART_GetFloat(int byteOfs) {
	byte buf[4];
	buf[0] = UART_GetByte(byteOfs);
	buf[1] = UART_GetByte(byteOfs+1);
	buf[2] = UART_GetByte(byteOfs+2);
	buf[3] = UART_GetByte(byteOfs+3);
	return *((float*)buf);
}
static int HGS02_TryToGetNextPacket(void) {
	int cs;
	int i;
	int c_garbage_consumed = 0;
	byte checksum;

	cs = UART_GetDataSize();

	if(cs < HGS02_UART_PACKET_LEN) {
		return 0;
	}
	// skip garbage data (should not happen)
	while(cs > 0) {
        if (UART_GetByte(0) != HGS02_PACKET_HEADER) {
			UART_ConsumeBytes(1);
			c_garbage_consumed++;
			cs--;
		} else {
			break;
		}
	}
	if(c_garbage_consumed > 0){
        ADDLOG_WARN(LOG_FEATURE_ENERGYMETER,
                    "Consumed %i unwanted non-header byte in HGS02 buffer\n",
                    c_garbage_consumed);
	}
	if(cs < HGS02_UART_PACKET_LEN) {
		return 0;
	}
    if (UART_GetByte(0) != HGS02_PACKET_HEADER)
		return 0;
	int temp = UART_GetByte(1) - 30;
	int hum = UART_GetByte(2);
	float co2 = UART_GetFloat(3); // bytes 3, 4, 5, 6
	float tvoc = UART_GetFloat(7); // bytes 7, 8, 9, 10
	float hcho = UART_GetFloat(11); // bytes 11, 12, 13, 14

	ADDLOG_WARN(LOG_FEATURE_ENERGYMETER,
		"Temp %i hum %i co2 %f tvoc %f hcho %f\n",
		temp, hum, co2, tvoc, hcho);

    UART_ConsumeBytes(HGS02_UART_PACKET_LEN);
	return HGS02_UART_PACKET_LEN;
}

// THIS IS called by 'startDriver HGS02' command
// You can set alternate baud with 'startDriver HGS02 9600' syntax
void HGS02_Init(void) {
	hgs02_baudRate = Tokenizer_GetArgIntegerDefault(1, 115200);

	UART_InitUART(hgs02_baudRate, 0);
	UART_InitReceiveRingBuffer(HGS02_RECEIVE_BUFFER_SIZE);
}

void HGS02_RunEverySecond(void) {
    HGS02_TryToGetNextPacket();

    UART_InitUART(hgs02_baudRate, 0);
}
