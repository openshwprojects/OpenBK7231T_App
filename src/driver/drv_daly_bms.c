#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../quicktick.h"
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"

#include "../mqtt/new_mqtt.h"
#include "errno.h"
#include <lwip/sockets.h>
#include "drv_uart.h"

#if ENABLE_DRIVER_DALY_BMS


static int g_baudRate = 9600;

static int writeRegister(int registerAddress,short value);

static SemaphoreHandle_t g_daly_mutex = 0;

bool DALY_BMS_Mutex_Take(int del) {
	int taken;

	if (g_daly_mutex == 0)
	{
		g_daly_mutex = xSemaphoreCreateMutex();
	}
	taken = xSemaphoreTake(g_daly_mutex, del);
	if (taken == pdTRUE) {
		return true;
	}
	return false;
}
void DALY_BMS_Mutex_Free() {
	xSemaphoreGive(g_daly_mutex);
}

void readCellVoltages(){

	if(!DALY_BMS_Mutex_Take(10)){
		return;
    }
	unsigned char buffer[13];
	buffer[0] = 0xA5; // header
	buffer[1] = 0x40; // address
	buffer[2] = 0x95; // read cell voltage command
	buffer[3] = 0x08; // length
	for(int i=4;i<12;i++){
		buffer[i]=0x00;
	}
	int checksum=0;
	for(int i=0;i<11;i++){
		checksum+=buffer[i];
	}
	buffer[12]=checksum;

	for(int i = 0; i < 8; i++)
	{
		UART_SendByte(buffer[i]);
	}
	MQTT_PublishMain_StringInt("daly_bms_debug", 1, 0);

	unsigned char receive_buffer[256];
	int len = UART_GetDataSize();
	int delay=0;

	while(len ==0 && delay < 250)
	{
		rtos_delay_milliseconds(1);
		len = UART_GetDataSize();
		delay++;
	}
	MQTT_PublishMain_StringInt("daly_bms_debug", 2, 0);

    for(int i = 0; i < len; i++)
    {
        receive_buffer[i] = UART_GetByte(i);
    }
	if(len==0){
		return;
	}
	float cellVoltage[6];
    UART_ConsumeBytes(len);
    DALY_BMS_Mutex_Free();
	MQTT_PublishMain_StringInt("daly_bms_debug", 3, 0);
	int cellNo=0;
	char tmp[23];
	for(int k=0;k<2;k++){
		for(int i=0;i<3;i++){
				cellVoltage[cellNo]=(float)(receive_buffer[5+i+i]<<8+receive_buffer[6+i+i]);

				sprintf(tmp, "daly_bms_cell_voltage_%d", cellNo);

				MQTT_PublishMain_StringFloat(tmp, cellVoltage[cellNo],2, 0);
				cellNo++;
		}
	}

	MQTT_PublishMain_StringInt("daly_bms_debug", 4, 0);
}


void DALY_BMS_RunEverySecond()
{
		readCellVoltages();
}


// startDriver ZK10022 [baudrate] [hw flow control] [stop_bits] [data_width]

// backlog stopDriver ZK10022; startDriver ZK10022 115200 0 0 3
void DALY_BMS_Init()
{

	UART_InitUART(g_baudRate, 0,0,3, false);
	UART_InitReceiveRingBuffer(512);

}
void DALY_BMS_Deinit(){
}




void DALY_BMS_AddCommands(void) {
}
#endif
