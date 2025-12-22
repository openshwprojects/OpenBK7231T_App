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

void sendRequest(byte address){
	unsigned char buffer[13];
	buffer[0] = 0xA5;
	buffer[1] = 0x40;
	buffer[2] = address;
	buffer[3] = 0x08;
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
}

int getUartDataSize(unsigned char* receiveBuffer){
	int len = UART_GetDataSize();
	int delay=0;

	while(len ==0 && delay < 250)
	{
		rtos_delay_milliseconds(1);
		len = UART_GetDataSize();
		delay++;
	}
	for(int i = 0; i < len; i++)
	{
		receive_buffer[i] = UART_GetByte(i);
	}
	UART_ConsumeBytes(len);
	return len;
}

void readCellBalanceState(){
	if(!DALY_BMS_Mutex_Take(10)){
		return;
	}
	sendRequest(0x97);

	unsigned char receive_buffer[256];
	int len= getUartDataSize(receive_buffer);

	DALY_BMS_Mutex_Free();

	if(len==0){
		return;
	}

	char tmp[30];
	for(int k=0;k<(len/13);k++){
		for(int i=0;i<6;i++){
			for(int cellIndex=0;cellIndex<8;celIndex++){
				int balancerState=receive_buffer[5+i]&(1<<cellIndex)==(1<<cellIndex)?1:0;
				sprintf(tmp, "daly_bms_balancer_state_%d", cellIndex+i*8);
				MQTT_PublishMain_StringInt(tmp, balancerState, 0);
			}
		}
	}
}

void readSocTotalVoltage(){
	if(!DALY_BMS_Mutex_Take(10)){
		return;
	}
	sendRequest(0x90);

	unsigned char receive_buffer[256];
	int len= getUartDataSize(receive_buffer);
	DALY_BMS_Mutex_Free();

	if(len==0){
		return;
	}
	DALY_BMS_Mutex_Free();

	char tmp[30];
	for(int k=0;k<(len/13);k++){
		float cumulativeVoltage=(receive_buffer[5]*256+receive_buffer[6])*0.1;
		MQTT_PublishMain_StringFloat("daly_bms_cum_voltage", cumulativeVoltage,1, 0);
		float gatherVoltage=(receive_buffer[7]*256+receive_buffer[8])*0.1;
		MQTT_PublishMain_StringFloat("daly_bms_gather_voltage", gatherVoltage,1, 0);
		float current=(receive_buffer[9]*256+receive_buffer[10])*0.1;
		MQTT_PublishMain_StringFloat("daly_bms_current", current,1, 0);
		float stateOfCharge=(receive_buffer[11]*256+receive_buffer[12])*0.1;
		MQTT_PublishMain_StringFloat("daly_bms_soc", stateOfCharge,1, 0);
	}
}

void readCellVoltages(){
	if(!DALY_BMS_Mutex_Take(10)){
		return;
	}
	sendRequest(0x95);

	unsigned char receive_buffer[256];
	int len= getUartDataSize(receive_buffer);
	DALY_BMS_Mutex_Free();

	if(len==0){
		return;
	}
    DALY_BMS_Mutex_Free();

	int cellNo=0;
	char tmp[30];
	for(int k=0;k<(len/13);k++){
		for(int i=0;i<3;i++){
				float cellVoltage= (receive_buffer[5+i+i+(k*13)]*256+receive_buffer[6+i+i+(k*13)])*0.001 ;
				sprintf(tmp, "daly_bms_cell_voltage_%d", cellNo);
				MQTT_PublishMain_StringFloat(tmp, cellVoltage,3, 0);
				cellNo++;
		}
	}
}


void DALY_BMS_RunEverySecond()
{
	readCellVoltages();
	readCellBalanceState();
	readSocTotalVoltage();
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
