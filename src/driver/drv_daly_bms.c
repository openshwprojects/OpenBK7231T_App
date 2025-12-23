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
static char g_errorString[768];

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

void readMosFet()
{
	if(!DALY_BMS_Mutex_Take(10)){
		return;
	}
	sendRequest(0x94);

	unsigned char receive_buffer[256];
	getUartDataSize(receive_buffer,13);
	DALY_BMS_Mutex_Free();

	switch(receive_buffer[4]){
		case 0:
            MQTT_PublishMain_StringString("daly_bms_status", "Stationary", 0);
			break;
		case 1:
            MQTT_PublishMain_StringString("daly_bms_status", "Charge", 0);
			break;
		case 2:
            MQTT_PublishMain_StringString("daly_bms_status", "Discharge", 0);
			break;
    }
    MQTT_PublishMain_StringInt("daly_bms_charge_fet_state", receive_buffer[5], 0);
    MQTT_PublishMain_StringInt("daly_bms_discharge_fet_state", receive_buffer[6], 0);
    MQTT_PublishMain_StringInt("daly_bms_heartbeat", receive_buffer[7], 0);
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

    int statusDIO=receive_buffer[8];

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
            if((k*13+tempIndex+1)>=g_noOfTempSensors){
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

    float cumulativeVoltage=(receive_buffer[4]*256+receive_buffer[5])*0.1;
    MQTT_PublishMain_StringFloat("daly_bms_cum_voltage", cumulativeVoltage,1, 0);

    float gatherVoltage=(receive_buffer[6]*256+receive_buffer[7])*0.1;
    MQTT_PublishMain_StringFloat("daly_bms_gather_voltage", gatherVoltage,1, 0);

    float current=(receive_buffer[8]*256+receive_buffer[9])*0.1-3000;
    MQTT_PublishMain_StringFloat("daly_bms_current", current,1, 0);

    float stateOfCharge=(receive_buffer[10]*256+receive_buffer[11])*0.1;
    MQTT_PublishMain_StringFloat("daly_bms_soc", stateOfCharge,1, 0);
}
void readMinMaxVoltage()
{
	if(!DALY_BMS_Mutex_Take(10)){
		return;
	}
	sendRequest(0x91);

	unsigned char receive_buffer[256];
	int len= getUartDataSize(receive_buffer,13);
	DALY_BMS_Mutex_Free();

    float maxCellV = (float)((receive_buffer[4] << 8) | receive_buffer[5])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_max_cell_voltage", maxCellV,3, 0);
    MQTT_PublishMain_StringInt("daly_bms_max_cell_num", receive_buffer[6], 0);
    float minCellV = (float)((receive_buffer[7] << 8) | receive_buffer[8])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_min_cell_voltage", maxCellV,3, 0);
    MQTT_PublishMain_StringInt("daly_bms_min_cell_num", receive_buffer[9], 0);
    MQTT_PublishMain_StringFloat("daly_bms_cell_dif", maxCellV-minCellV,3, 0);
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

    float maxCellThreshold1 = (float)((receive_buffer[4] << 8) | receive_buffer[5])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_max_cell_threshold_1", maxCellThreshold1,3, 0);
    float maxCellThreshold2 = (float)((receive_buffer[6] << 8) | receive_buffer[7])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_max_cell_threshold_2", maxCellThreshold2,3, 0);
    float minCellThreshold1 = (float)((receive_buffer[8] << 8) | receive_buffer[9])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_min_cell_threshold_1", minCellThreshold1,3, 0);
    float minCellThreshold2 = (float)((receive_buffer[10] << 8) | receive_buffer[11])*0.001;
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

    float maxPackThreshold1 = (float)((receive_buffer[4] << 8) | receive_buffer[5])*0.1;
    MQTT_PublishMain_StringFloat("daly_bms_max_pack_voltage_threshold_1", maxPackThreshold1,1, 0);
    float maxPackThreshold2 = (float)((receive_buffer[6] << 8) | receive_buffer[7])*0.1;
    MQTT_PublishMain_StringFloat("daly_bms_max_pack_voltage_threshold_2", maxPackThreshold2,1, 0);
    float minPackThreshold1 = (float)((receive_buffer[8] << 8) | receive_buffer[9])*0.1;
    MQTT_PublishMain_StringFloat("daly_bms_min_pack_voltage_threshold_1", minPackThreshold1,1, 0);
    float minPackThreshold2 = (float)((receive_buffer[10] << 8) | receive_buffer[11])*0.1;
    MQTT_PublishMain_StringFloat("daly_bms_min_pack_voltage_threshold_2", minPackThreshold2,1, 0);
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

    float maxCurrentThreshold1 = (float)((receive_buffer[4] << 8) | receive_buffer[5])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_max_current_threshold_1", maxCurrentThreshold1,3, 0);
    float maxCurrentThreshold2 = (float)((receive_buffer[6] << 8) | receive_buffer[7])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_max_current_threshold_2", maxCurrentThreshold2,3, 0);
    float minCurrentThreshold1 = (float)((receive_buffer[8] << 8) | receive_buffer[9])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_min_current_threshold_1", minCurrentThreshold1,3, 0);
    float minCurrentThreshold2 = (float)((receive_buffer[10] << 8) | receive_buffer[11])*0.001;
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

    MQTT_PublishMain_StringInt("daly_bms_max_charge_temp_1", receive_buffer[4]-40, 0);
    MQTT_PublishMain_StringInt("daly_bms_max_charge_temp_2", receive_buffer[5]-40, 0);
    MQTT_PublishMain_StringInt("daly_bms_min_charge_temp_1", receive_buffer[6]-40, 0);
    MQTT_PublishMain_StringInt("daly_bms_min_charge_temp_2", receive_buffer[7]-40, 0);

    MQTT_PublishMain_StringInt("daly_bms_max_discharge_temp_1", receive_buffer[8]-40, 0);
    MQTT_PublishMain_StringInt("daly_bms_max_discharge_temp_2", receive_buffer[9]-40, 0);
    MQTT_PublishMain_StringInt("daly_bms_min_discharge_temp_1", receive_buffer[10]-40, 0);
    MQTT_PublishMain_StringInt("daly_bms_min_discharge_temp_2", receive_buffer[11]-40, 0);
}
void readChargeThreshold()
{
	if(!DALY_BMS_Mutex_Take(10)){
		return;
	}
	sendRequest(0x5D);

	unsigned char receive_buffer[256];
	int len= getUartDataSize(receive_buffer,13);
	DALY_BMS_Mutex_Free();

    float maxChargeCurrentThreshold1 = (float)((receive_buffer[4] << 8) | receive_buffer[5])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_max_charge_current_threshold_1", maxChargeCurrentThreshold1,3, 0);
    float maxChargeCurrentThreshold2 = (float)((receive_buffer[6] << 8) | receive_buffer[7])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_max_charge_current_threshold_2", maxChargeCurrentThreshold2,3, 0);
    float minChargeCurrentThreshold1 = (float)((receive_buffer[8] << 8) | receive_buffer[9])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_min_charge_current_threshold_1", minChargeCurrentThreshold1,3, 0);
    float minChargeCurrentThreshold2 = (float)((receive_buffer[10] << 8) | receive_buffer[11])*0.001;
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
bool bitRead(byte in,int bitIndex){
    return in & (1<<bitIndex) == (1<<bitIndex);
}
void readFailureCodes()
{

	if(!DALY_BMS_Mutex_Take(10)){
		return;
	}
	sendRequest(0x98);

	unsigned char receive_buffer[256];
	int len= getUartDataSize(receive_buffer,13);
	DALY_BMS_Mutex_Free();

    memset(g_errorString, '\0', sizeof(g_errorString));
    if (bitRead(receive_buffer[4], 1))
        strcat(g_errorString,"Cell v h l2,");
    else if (bitRead(receive_buffer[4], 0))
        strcat(g_errorString,"Cell v h l1,");
    if (bitRead(receive_buffer[4], 3))
        strcat(g_errorString,"Cell v l l2,");
    else if (bitRead(receive_buffer[4], 2))
        strcat(g_errorString,"Cell v l l1,");
    if (bitRead(receive_buffer[4], 5))
        strcat(g_errorString,"Sum v h l2,");
    else if (bitRead(receive_buffer[4], 4))
        strcat(g_errorString,"Sum v h l1,");
    if (bitRead(receive_buffer[4], 7))
        strcat(g_errorString,"Sum v l l2,");
    else if (bitRead(receive_buffer[4], 6))
        strcat(g_errorString,"Sum v l l1,");
    /* 0x01 */
    if (bitRead(receive_buffer[5], 1))
        strcat(g_errorString,"C t h l2,");
    else if (bitRead(receive_buffer[5], 0))
        strcat(g_errorString,"C t h l1,");
    if (bitRead(receive_buffer[5], 3))
        strcat(g_errorString,"C t l l2,");
    else if (bitRead(receive_buffer[5], 2))
        strcat(g_errorString,"C t l l1,");
    if (bitRead(receive_buffer[5], 5))
        strcat(g_errorString,"DC t h l2,");
    else if (bitRead(receive_buffer[5], 4))
        strcat(g_errorString,"DC t h l1,");
    if (bitRead(receive_buffer[5], 7))
        strcat(g_errorString,"DC t l l2,");
    else if (bitRead(receive_buffer[5], 6))
        strcat(g_errorString,"DC t l l1,");
    /* 0x02 */
    if (bitRead(receive_buffer[6], 1))
        strcat(g_errorString,"C OC l1,");
    else if (bitRead(receive_buffer[6], 0))
        strcat(g_errorString,"C OC l1,");
    if (bitRead(receive_buffer[6], 3))
        strcat(g_errorString,"DC OC l2,");
    else if (bitRead(receive_buffer[6], 2))
        strcat(g_errorString,"DC OC l1,");
    if (bitRead(receive_buffer[6], 5))
        strcat(g_errorString,"SOC h l2,");
    else if (bitRead(receive_buffer[6], 4))
        strcat(g_errorString,"SOC h l1,");
    if (bitRead(receive_buffer[6], 7))
        strcat(g_errorString,"SOC l l2,");
    else if (bitRead(receive_buffer[6], 6))
        strcat(g_errorString,"SOC l l1,");
	if(strlen(g_errorString)>300){
        MQTT_PublishMain_StringFloat("daly_bms_min_charge_current_threshold_2", minChargeCurrentThreshold2,3, 0);
		return;
    }
    /* 0x03 */
    if (bitRead(receive_buffer[7], 1))
        strcat(g_errorString,"Dff v l2,");
    else if (bitRead(receive_buffer[7], 0))
        strcat(g_errorString,"Dff v l1,");
    if (bitRead(receive_buffer[7], 3))
        strcat(g_errorString,"Dff t l2,");
    else if (bitRead(receive_buffer[7], 2))
        strcat(g_errorString,"Dff t l1,");
    /* 0x04 */
    if (bitRead(receive_buffer[8], 0))
        strcat(g_errorString,"C MOS t h alarm,");
    if (bitRead(receive_buffer[8], 1))
        strcat(g_errorString,"DC MOS t h alarm,");
    if (bitRead(receive_buffer[8], 2))
        strcat(g_errorString,"C MOS t sen err,");
    if (bitRead(receive_buffer[8], 3))
        strcat(g_errorString,"DC MOS t sen err,");
    if (bitRead(receive_buffer[8], 4))
        strcat(g_errorString,"C MOS adh err,");
    if (bitRead(receive_buffer[8], 5))
        strcat(g_errorString,"DC MOS adh err,");
    if (bitRead(receive_buffer[8], 6))
        strcat(g_errorString,"C MOS open circuit err,");
    if (bitRead(receive_buffer[8], 7))
        strcat(g_errorString,"DC MOS open circuit err,");
    /* 0x05 */
    if (bitRead(receive_buffer[9], 0))
        strcat(g_errorString,"AFE collect chip err,");
    if (bitRead(receive_buffer[9], 1))
        strcat(g_errorString,"V collect dropped,");
    if (bitRead(receive_buffer[9], 2))
        strcat(g_errorString,"Cell t sensor err,");
    if (bitRead(receive_buffer[9], 3))
        strcat(g_errorString,"EEPROM err,");
    if (bitRead(receive_buffer[9], 4))
        strcat(g_errorString,"RTC err,");
    if (bitRead(receive_buffer[9], 5))
        strcat(g_errorString,"Precharge fail,");
    if (bitRead(receive_buffer[9], 6))
        strcat(g_errorString,"Comm fail,");
    if (bitRead(receive_buffer[9], 7))
        strcat(g_errorString,"Int comm fail,");
    /* 0x06 */
    if (bitRead(receive_buffer[10], 0))
        strcat(g_errorString,"Current module fault,");
    if (bitRead(receive_buffer[10], 1))
        strcat(g_errorString,"S V detect fault,");
    if (bitRead(receive_buffer[10], 2))
        strcat(g_errorString,"S C protect fault,");
    if (bitRead(receive_buffer[10], 3))
        strcat(g_errorString,"L V forb chg fault,");

    MQTT_PublishMain_StringFloat("daly_bms_min_charge_current_threshold_2", minChargeCurrentThreshold2,3, 0);
    return;
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

    readFailureCodes();
    readMinMaxVoltage();

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


void sendSetRequest(byte address,const byte* setBuffer){
	unsigned char buffer[13];
	buffer[0] = 0xA5;
	buffer[1] = 0x40;
	buffer[2] = address;
	buffer[3] = 0x08;
	for(int i=4;i<12;i++){
		buffer[i]=setBuffer[i-4];
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



void setCellVoltageThreshold(float voltageMinWarn,float voltageMinOff,float voltageMaxWarn,float voltageMaxOff)
{
	if(!DALY_BMS_Mutex_Take(10)){
		return;
	}
	byte set_buffer[8];
	set_buffer[0]= ((int) (voltageMaxWarn*1000))>>8;
	set_buffer[1]= ((int) (voltageMaxWarn*1000));

	set_buffer[2]= ((int) (voltageMaxOff*1000))>>8;
	set_buffer[3]= ((int) (voltageMaxOff*1000));

	set_buffer[4]= ((int) (voltageMinWarn*1000))>>8;
	set_buffer[5]= ((int) (voltageMinWarn*1000));

	set_buffer[6]= ((int) (voltageMinOff*1000))>>8;
	set_buffer[7]= ((int) (voltageMinOff*1000));

	sendSetRequest(0x59,set_buffer);

	unsigned char receive_buffer[256];

	int len= getUartDataSize(receive_buffer,13);
	DALY_BMS_Mutex_Free();

    float maxCellThreshold1 = (float)((receive_buffer[4] << 8) | receive_buffer[5])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_max_cell_threshold_1", maxCellThreshold1,3, 0);
    float maxCellThreshold2 = (float)((receive_buffer[6] << 8) | receive_buffer[7])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_max_cell_threshold_2", maxCellThreshold2,3, 0);
    float minCellThreshold1 = (float)((receive_buffer[8] << 8) | receive_buffer[9])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_min_cell_threshold_1", minCellThreshold1,3, 0);
    float minCellThreshold2 = (float)((receive_buffer[10] << 8) | receive_buffer[11])*0.001;
    MQTT_PublishMain_StringFloat("daly_bms_min_cell_threshold_2", minCellThreshold2,3, 0);
}


commandResult_t CMD_DALY_BMS_Set_Voltage_Thresholds(const void* context, const char* cmd, const char* args, int cmdFlags) {

	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() == 4) {
		float vMin1 = Tokenizer_GetArgFloat(0);
		float vMin2 = Tokenizer_GetArgFloat(1);
		float vMax1 = Tokenizer_GetArgFloat(2);
		float vMax2 = Tokenizer_GetArgFloat(3);
        setCellVoltageThreshold(vMin1,vMin2,vMax1,vMax2);
        return CMD_RES_OK;

	}
    return CMD_RES_ERROR;
}


void DALY_BMS_AddCommands(void) {

	CMD_RegisterCommand("DalyBms_Set_Cell_Voltage_Thresholds", CMD_DALY_BMS_Set_Voltage_Thresholds, NULL);
}
#endif
