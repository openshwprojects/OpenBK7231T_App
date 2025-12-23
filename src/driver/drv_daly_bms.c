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
static int g_noOfCells=0;
static int g_noOfTempSensors=0;
static int g_currentIndex=0;
static bool g_firstIteration=true;

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

int getUartDataSize(unsigned char* receive_buffer,int expected_length){
    rtos_delay_milliseconds(10);
	int len = UART_GetDataSize();
	int delay=0;

	while(len <expected_length && delay < 250)
	{
		rtos_delay_milliseconds(2);
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

    rtos_delay_milliseconds(20);
	unsigned char receive_buffer[256];
	int len= getUartDataSize(receive_buffer,13);

	DALY_BMS_Mutex_Free();

	if(len==0){
		return;
	}

	char tmp[30];
	for(int k=0;k<(len/13);k++){
		for(int i=0;i<6;i++){
			for(int cellIndex=0;cellIndex<8;cellIndex++){
				int balancerState=receive_buffer[4+i]&(1<<cellIndex)==(1<<cellIndex)?1:0;
				sprintf(tmp, "daly_bms_balancer_state_%d", cellIndex+i*8);
				MQTT_PublishMain_StringInt(tmp, balancerState, 0);

				if(cellIndex+i*8>=g_noOfCells){
					return;
				}
			}
		}
	}
}
void readStatusInformation(){
	if(!DALY_BMS_Mutex_Take(10)){
		return;
	}
	sendRequest(0x94);

	unsigned char receive_buffer[256];
	int len= getUartDataSize(receive_buffer,13);

	DALY_BMS_Mutex_Free();

	if(len==0){
		return;
	}

    int statusDIO=receive_buffer[8]

    MQTT_PublishMain_StringInt("daly_bms_no_of_batteries", receive_buffer[4], 0);
    MQTT_PublishMain_StringInt("daly_bms_no_of_temperatures", receive_buffer[5], 0);
    MQTT_PublishMain_StringInt("daly_bms_charge_status", receive_buffer[6], 0);
    MQTT_PublishMain_StringInt("daly_bms_load_status", receive_buffer[7], 0);
    MQTT_PublishMain_StringInt("daly_bms_cycles", receive_buffer[9]*256+receive_buffer[10], 0);
}
void readCellTemperature(){
	if(!DALY_BMS_Mutex_Take(10)){
		return;
	}
	sendRequest(0x96);

	unsigned char receive_buffer[256];
	int len= getUartDataSize(receive_buffer,(g_noOfTempSensors/7+1)*13);

	DALY_BMS_Mutex_Free();

	if(len==0){
		return;
	}

	char tmp[30];
	for(int k=0;k<(len/13);k++){
        for(int tempIndex=0;tempIndex<8;tempIndex++){
            int temperature=receive_buffer[k*13+5+tempIndex]-40;
            sprintf(tmp, "daly_bms_temperature_%d", k*13+tempIndex);
            MQTT_PublishMain_StringInt(tmp, temperature, 0);
            if(k*13+tempIndex>=g_noOfTempSensors){
                return;
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
	int len= getUartDataSize(receive_buffer,13);
	DALY_BMS_Mutex_Free();

	if(len==0){
		return;
	}

    MQTT_PublishMain_StringInt("daly_bms_debug_len", len, 0);
	for(int k=0;k<(len/13);k++){
        MQTT_PublishMain_StringInt("daly_bms_debug_frame_number", receive_buffer[4+k*13], 0);

		float cumulativeVoltage=(receive_buffer[4]*256+receive_buffer[5])*0.1;
		MQTT_PublishMain_StringFloat("daly_bms_cum_voltage", cumulativeVoltage,1, 0);

		float gatherVoltage=(receive_buffer[6]*256+receive_buffer[7])*0.1;
		MQTT_PublishMain_StringFloat("daly_bms_gather_voltage", gatherVoltage,1, 0);

		float current=(receive_buffer[8]*256+receive_buffer[9])*0.1-30000;
		MQTT_PublishMain_StringFloat("daly_bms_current", current,1, 0);

		float stateOfCharge=(receive_buffer[10]*256+receive_buffer[11])*0.1;
		MQTT_PublishMain_StringFloat("daly_bms_soc", stateOfCharge,1, 0);
	}
}
void readCellVoltageThreshold()
{
	if(!DALY_BMS_Mutex_Take(10)){
		return;
	}
	sendRequest(0x59);

	unsigned char receive_buffer[256];
	int len= getUartDataSize(receive_buffer,13);
	DALY_BMS_Mutex_Free();

    float maxCellThreshold1 = (float)((receive_buffer[0][4] << 8) | receive_buffer[0][5])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_max_cell_threshold_1", maxCellThreshold1,3, 0);
    float maxCellThreshold2 = (float)((receive_buffer[0][6] << 8) | receive_buffer[0][7])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_max_cell_threshold_2", maxCellThreshold2,3, 0);
    float minCellThreshold1 = (float)((receive_buffer[0][8] << 8) | receive_buffer[0][9])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_min_cell_threshold_1", minCellThreshold1,3, 0);
    float minCellThreshold2 = (float)((receive_buffer[0][10] << 8) | receive_bufferf[0][11])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_min_cell_threshold_2", minCellThreshold2,3, 0);
}
void readPackVoltageThreshold()
{
	if(!DALY_BMS_Mutex_Take(10)){
		return;
	}
	sendRequest(0x5A);

	unsigned char receive_buffer[256];
	int len= getUartDataSize(receive_buffer,13);
	DALY_BMS_Mutex_Free();

    float maxPackThreshold1 = (float)((receive_buffer[0][4] << 8) | receive_buffer[0][5])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_max_pack_voltage_threshold_1", maxPackThreshold1,3, 0);
    float maxPackThreshold2 = (float)((receive_buffer[0][6] << 8) | receive_buffer[0][7])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_max_pack_voltage_threshold_2", maxPackThreshold2,3, 0);
    float minPackThreshold1 = (float)((receive_buffer[0][8] << 8) | receive_buffer[0][9])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_min_pack_voltage_threshold_1", minPackThreshold1,3, 0);
    float minPackThreshold2 = (float)((receive_buffer[0][10] << 8) | receive_bufferf[0][11])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_min_pack_voltage_threshold_2", minPackThreshold2,3, 0);
}
void readCurrentThreshold()
{
	if(!DALY_BMS_Mutex_Take(10)){
		return;
	}
	sendRequest(0x5B);

	unsigned char receive_buffer[256];
	int len= getUartDataSize(receive_buffer,13);
	DALY_BMS_Mutex_Free();

    float maxCurrentThreshold1 = (float)((receive_buffer[0][4] << 8) | receive_buffer[0][5])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_max_current_threshold_1", maxCurrentThreshold1,3, 0);
    float maxCurrentThreshold2 = (float)((receive_buffer[0][6] << 8) | receive_buffer[0][7])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_max_current_threshold_2", maxCurrentThreshold2,3, 0);
    float minCurrentThreshold1 = (float)((receive_buffer[0][8] << 8) | receive_buffer[0][9])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_min_current_threshold_1", minCurrentThreshold1,3, 0);
    float minCurrentThreshold2 = (float)((receive_buffer[0][10] << 8) | receive_bufferf[0][11])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_min_current_threshold_2", minCurrentThreshold2,3, 0);
}
void readTemperatureThreshold()
{
	if(!DALY_BMS_Mutex_Take(10)){
		return;
	}
	sendRequest(0x5C);

	unsigned char receive_buffer[256];
	int len= getUartDataSize(receive_buffer,13);
	DALY_BMS_Mutex_Free();

    MQTT_PublishMain_StringInt("daly_bms_max_charge_temp_1", receive_buffer[0][4]-40, 0);
    MQTT_PublishMain_StringInt("daly_bms_max_charge_temp_2", receive_buffer[0][5]-40, 0);
    MQTT_PublishMain_StringInt("daly_bms_min_charge_temp_1", receive_buffer[0][6]-40, 0);
    MQTT_PublishMain_StringInt("daly_bms_min_charge_temp_2", receive_buffer[0][7]-40, 0);

    MQTT_PublishMain_StringInt("daly_bms_max_discharge_temp_1", receive_buffer[0][8]-40, 0);
    MQTT_PublishMain_StringInt("daly_bms_max_discharge_temp_2", receive_buffer[0][9]-40, 0);
    MQTT_PublishMain_StringInt("daly_bms_min_discharge_temp_1", receive_buffer[0][10]-40, 0);
    MQTT_PublishMain_StringInt("daly_bms_min_discharge_temp_2", receive_buffer[0][11]-40, 0);
}
void readChargeThreshold()
{
	if(!DALY_BMS_Mutex_Take(10)){
		return;
	}
	sendRequest(0x5D);

    rtos_delay_milliseconds(20);
	unsigned char receive_buffer[256];
	int len= getUartDataSize(receive_buffer,13);
	DALY_BMS_Mutex_Free();

    float maxChargeCurrentThreshold1 = (float)((receive_buffer[0][4] << 8) | receive_buffer[0][5])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_max_charge_current_threshold_1", maxChargeCurrentThreshold1,3, 0);
    float maxChargeCurrentThreshold2 = (float)((receive_buffer[0][6] << 8) | receive_buffer[0][7])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_max_charge_current_threshold_2", maxChargeCurrentThreshold2,3, 0);
    float minChargeCurrentThreshold1 = (float)((receive_buffer[0][8] << 8) | receive_buffer[0][9])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_min_charge_current_threshold_1", minChargeCurrentThreshold1,3, 0);
    float minChargeCurrentThreshold2 = (float)((receive_buffer[0][10] << 8) | receive_bufferf[0][11])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_min_charge_current_threshold_2", minChargeCurrentThreshold2,3, 0);
}

void readCellVoltages(){
	if(!DALY_BMS_Mutex_Take(10)){
		return;
	}
	sendRequest(0x95);

	unsigned char receive_buffer[256];
	int len= getUartDataSize(receive_buffer,g_noOfCells/3*13);
	DALY_BMS_Mutex_Free();

	if(len==0){
		return;
	}
    DALY_BMS_Mutex_Free();

	int cellNo=0;
	char tmp[30];
    MQTT_PublishMain_StringInt("daly_bms_debug_len", len, 0);
	for(int k=0;k<(len/13);k++){
        MQTT_PublishMain_StringInt("daly_bms_debug_frame_number", receive_buffer[4+k*13], 0);
		for(int i=0;i<3;i++){
				float cellVoltage= (receive_buffer[5+i+i+(k*13)]*256+receive_buffer[6+i+i+(k*13)])*0.001 ;
				sprintf(tmp, "daly_bms_cell_voltage_%d", cellNo);
				MQTT_PublishMain_StringFloat(tmp, cellVoltage,3, 0);
				cellNo++;
				if(cellNo>=g_noOfCells){
					return;
				}
		}
	}
}


void DALY_BMS_RunEverySecond()
{
	if(g_firstIteration){
		readCellVoltageThreshold();
		readStatusInformation();
		readPackVoltageThreshold();
		readCurrentThreshold();
		readChargeThreshold();
        readTemperatureThreshold();
        g_firstIteration=false;
        return;
    }
    if(g_currentIndex==0){
        readCellVoltages();
    }
    if(g_currentIndex==1){
        readCellBalanceState();
    }
    if(g_currentIndex==2){
        readSocTotalVoltage();
    }
    if(g_currentIndex==4){
        readStatusInformation();
    }
    if(g_currentIndex==4){
        readCellTemperature();
    }
    g_currentIndex++;
    if(g_currentIndex==5){
        g_currentIndex=0;
    }
}


// backlog stopDriver DalyBms; startDriver DalyBms #noOfCells
void DALY_BMS_Init()
{

	g_noOfCells = Tokenizer_GetArgIntegerDefault(1, 0);
	g_noOfTempSensors = Tokenizer_GetArgIntegerDefault(2, 2);
	UART_InitUART(g_baudRate, 0,0,3, false);
	UART_InitReceiveRingBuffer(512);
	g_firstIteration=true;

}
void DALY_BMS_Deinit(){
}




void DALY_BMS_AddCommands(void) {
}
#endif
