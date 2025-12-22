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

	for(int i = 0; i < 13; i++)
	{
		UART_SendByte(buffer[i]);
	}
	MQTT_PublishMain_StringInt("daly_bms_debug", 1, 0);

	int len = UART_GetDataSize();
	int delay=0;

	while(len ==0 && delay < 250)
	{
		rtos_delay_milliseconds(1);
		len = UART_GetDataSize();
		delay++;
	}
	MQTT_PublishMain_StringInt("daly_bms_debug", 2, 0);
	MQTT_PublishMain_StringInt("daly_bms_debug_len", len, 0);

	unsigned char receive_buffer[256];
    for(int i = 0; i < len; i++)
    {
        receive_buffer[i] = UART_GetByte(i);
    }
    UART_ConsumeBytes(len);
	DALY_BMS_Mutex_Free();

	if(len==0){
		return;
	}
	MQTT_PublishMain_StringInt("daly_bms_debug_recv", receive_buffer[0], 0);
	MQTT_PublishMain_StringInt("daly_bms_debug_recv", receive_buffer[1], 0);
	MQTT_PublishMain_StringInt("daly_bms_debug_recv", receive_buffer[2], 0);
	MQTT_PublishMain_StringInt("daly_bms_debug_recv", receive_buffer[3], 0);
	MQTT_PublishMain_StringInt("daly_bms_debug_recv", receive_buffer[4], 0);
	MQTT_PublishMain_StringInt("daly_bms_debug_recv", receive_buffer[5], 0);
	MQTT_PublishMain_StringInt("daly_bms_debug_recv", receive_buffer[6], 0);
	float cellVoltage[6];
    DALY_BMS_Mutex_Free();
	MQTT_PublishMain_StringInt("daly_bms_debug", 3, 0);
	int cellNo=0;
	char tmp[30];
	for(int k=0;k<2;k++){
		for(int i=0;i<3;i++){
				//Response
				// 0xA5 // StartFlag
				// 0x01 // Address
				// 0x01 // Data Id
				// 0x08 // Data Length
				// Frame
				// 0x01 // Index
				// 3x2Bytes volrages // voltages // Start at Index 5
				// 1 Byte padding
				// Start of next frame 12


				cellVoltage[cellNo]=(float)(receive_buffer[5+i+(k*8)]<<8+receive_buffer[6+i+(k*8)]);

				sprintf(tmp, "daly_bms_cell_voltage_%d", cellNo);

				MQTT_PublishMain_StringFloat(tmp, cellVoltage[cellNo],0, 0);
				cellNo++;
		}
	}

	MQTT_PublishMain_StringInt("daly_bms_debug", 4, 0);
}


void DALY_BMS_RunEverySecond()
{
		readCellVoltages();
}


// backlog stopDriver DalyBms; startDriver DalyBms
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
