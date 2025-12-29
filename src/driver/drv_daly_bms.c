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
static int g_heatingPin=-1;
static int g_heatingPin_n=-1;
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
void emptyUart(){
	int delay=0;
    while(delay<200){
        rtos_delay_milliseconds(1);
        int len = UART_GetDataSize();
        UART_ConsumeBytes(len);
        delay++;
    }
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
	bool heat=true;

	char tmp[30];
	for(int k=0;k<(len/13);k++){
        for(int tempIndex=0;tempIndex<8;tempIndex++){
            int temperature=receive_buffer[k*13+5+tempIndex]-40;
			if(temperature>10){
				heat=false;
			}
            sprintf(tmp, "daly_bms_temperature_%d", k*13+tempIndex);
            MQTT_PublishMain_StringInt(tmp, temperature, 0);
            if((k*13+tempIndex+1)>=g_noOfTempSensors){
                return;
            }
		}
	}
	if(g_heatingPin != -1 && g_heatingPin_n != -1){
        sprintf(tmp, "daly_bms_heating", heat?1:0);
        HAL_PIN_SetOutputValue(g_heatingPin,heat?1:0);
        HAL_PIN_SetOutputValue(g_heatingPin_n,0);
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
    MQTT_PublishMain_StringFloat("daly_bms_min_cell_voltage", minCellV,3, 0);
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
	if(len==0){
        return;
    }

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
        MQTT_PublishMain_StringString("daly_bms_error", g_errorString, 0);
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

    MQTT_PublishMain_StringString("daly_bms_error", g_errorString, 0);
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
    readCellTemperature();

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
	g_heatingPin=PIN_FindPinIndexForRole(IOR_BAT_Relay, -1);
	g_heatingPin_n=PIN_FindPinIndexForRole(IOR_BAT_Relay_n, -1);
}
void DALY_BMS_Deinit(){
}


static uint16_t MODBUS_CRC16( const unsigned char *buf, unsigned int len )
{
	static const uint16_t table[256] = {
	0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
	0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
	0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
	0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
	0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
	0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
	0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
	0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
	0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
	0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
	0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
	0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
	0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
	0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
	0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
	0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
	0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
	0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
	0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
	0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
	0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
	0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
	0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
	0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
	0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
	0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
	0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
	0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
	0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
	0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
	0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
	0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040 };

	uint8_t xor = 0;
	uint16_t crc = 0xFFFF;

	while(len--)
	{
		xor = (*buf++) ^ crc;
		crc >>= 8;
		crc ^= table[xor];
	}

	return crc;
}

void setCommand06(short address,short value){
	unsigned char buffer[8];
	buffer[0] = 0x81;
	buffer[1] = 0x06;
	buffer[2] = address>>8;
	buffer[3] = address;

    buffer[4] = value>>8;
    buffer[5] = value;
	uint16_t crc = MODBUS_CRC16(buffer, 6);

	buffer[6] = crc & 0xFF;
	buffer[7] = (crc >> 8) & 0xFF;

	if(!DALY_BMS_Mutex_Take(100)){
		return;
	}
	for(int i = 0; i < 8; i++)
	{
		UART_SendByte(buffer[i]);
	}
    emptyUart();
	DALY_BMS_Mutex_Free();
}
void setCommand1003(short address,int value1,int value2){
	unsigned char buffer[14];
	buffer[0] = 0x81;
	buffer[1] = 0x10;
	buffer[2] = address>>8;
	buffer[3] = address;
	buffer[4] = 0x00;
	buffer[5] = 0x03;

    buffer[6] = value1>>8;
    buffer[7] = value1;

    buffer[8] = value2>>8;
    buffer[9] = value2;

    buffer[10] =value1>>8;
    buffer[11] = value1;


	uint16_t crc = MODBUS_CRC16(buffer, 12);

	buffer[12] = crc & 0xFF;
	buffer[13] = (crc >> 8) & 0xFF;

	if(!DALY_BMS_Mutex_Take(100)){
		return;
	}
	for(int i = 0; i < 14; i++)
	{
		UART_SendByte(buffer[i]);
	}
    emptyUart();
	DALY_BMS_Mutex_Free();
}
void setCommand1003_ex(short address,int value1,int value2,int value3){
	unsigned char buffer[14];
	buffer[0] = 0x81;
	buffer[1] = 0x10;
	buffer[2] = address>>8;
	buffer[3] = address;
	buffer[4] = 0x00;
	buffer[5] = 0x03;

    buffer[6] = value1>>8;
    buffer[7] = value1;

    buffer[8] = value2>>8;
    buffer[9] = value2;

    buffer[10] =value3>>8;
    buffer[11] = value3;


	uint16_t crc = MODBUS_CRC16(buffer, 12);

	buffer[12] = crc & 0xFF;
	buffer[13] = (crc >> 8) & 0xFF;

	if(!DALY_BMS_Mutex_Take(100)){
		return;
	}
	for(int i = 0; i < 14; i++)
	{
		UART_SendByte(buffer[i]);
	}
    emptyUart();
	DALY_BMS_Mutex_Free();
}
void setCommand1002(short address,int value1,int value2){
	unsigned char buffer[12];
	buffer[0] = 0x81;
	buffer[1] = 0x10;
	buffer[2] = address>>8;
	buffer[3] = address;
	buffer[4] = 0x00;
	buffer[5] = 0x02;

    buffer[6] = value1>>8;
    buffer[7] = value1;

    buffer[8] = value2>>8;
    buffer[9] = value2;

	uint16_t crc = MODBUS_CRC16(buffer, 10);

	buffer[10] = crc & 0xFF;
	buffer[11] = (crc >> 8) & 0xFF;

	if(!DALY_BMS_Mutex_Take(100)){
		return;
	}
	for(int i = 0; i < 12; i++)
	{
		UART_SendByte(buffer[i]);
	}
    emptyUart();
	DALY_BMS_Mutex_Free();
}

void setMaxCellVoltage(float voltage){
    setCommand06(0x032d,voltage*1000-200);
    setCommand1003(0x0130,voltage*1000-100,voltage*1000);
    setCommand1002(0x01c3,voltage*1000+50,voltage*1000-50);
}
void setMinCellVoltage(float voltage){
    setCommand06(0x032e,voltage*1000+200);
    setCommand1003(0x0134,voltage*1000+100,voltage*1000);
    setCommand1002(0x01c6,voltage*1000-100,voltage*1000);
}

void setMaxPackVoltage(float voltage){
    setCommand06(0x032f,voltage*10-12);
    setCommand1003(0x0138,voltage*10+6,voltage*10);
    setCommand1002(0x01c9,voltage*10+3,voltage*10-3);
}

void setMinPackVoltage(float voltage){
    setCommand06(0x0330,voltage*10+6);
    setCommand1003(0x013c,voltage*10+6,voltage*10);
    setCommand1002(0x01cc,voltage*10-6,voltage*10+6);
}

void setChargeOvercurrent(float current){
    setCommand06(0x0331,30000-(current*10*0.8-20));
    setCommand1002(0x0140,30000-(current*10*0.8),30000-(current*10));
    setCommand06(0x0143,30000-(current*10*1.33));
}
void setDischargeOvercurrent(float current){
    setCommand06(0x0332,30000+(current*10*0.8-20));
    setCommand1002(0x0145,30000+(current*10*0.8),30000+(current*10));
    setCommand06(0x0148,30000+(current*10*1.33));
}

void setMaxDiffVoltage(float voltage){
    setCommand1003(0x015a,voltage*1000-300,voltage*1000);
    setCommand1002(0x01db,voltage*1000+200,voltage*1000);
}

void setMaxChargeTemp(int temp){
    setCommand06(0x0333,temp+40-15);
    setCommand1003_ex(0x014a,temp+40-10,temp+40,temp+40-5);
    setCommand1002(0x01cf,temp+40+10,temp+40+5);
}

void setMinChargeTemp(int temp){
    setCommand06(0x0334,temp+40+10);
    setCommand1003(0x014e,temp+40+5,temp+40);
    setCommand1002(0x01d2,temp+40-5,temp+40);
}

void setMaxDischargeTemp(int temp){
    setCommand06(0x0335,temp+40-10);
    setCommand1003(0x0152,temp+40-5,temp+40);
    setCommand1002(0x01d5,temp+40+5,temp+40);
}
void setMinDischargeTemp(int temp){
    setCommand06(0x0336,temp+40+10);
    setCommand1003(0x0156,temp+40+5,temp+40);
    setCommand1002(0x01d8,temp+40-5,temp+40);
}
void setChargeSwitch(int status){
    setCommand06(0x0121,status);
}
void setDischargeSwitch(int status){
    setCommand06(0x0122,status);
}
void setBalanceSwitch(int status){
    setCommand06(0x0119,status);
}


commandResult_t CMD_DALY_BMS_Set_Cell_Voltage_Thresholds(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() == 2) {
		float vMin = Tokenizer_GetArgFloat(0);
		float vMax = Tokenizer_GetArgFloat(1);
        setMaxCellVoltage(vMax);
        setMinCellVoltage(vMin);
        g_firstIteration=true;
        return CMD_RES_OK;
	}
    return CMD_RES_NOT_ENOUGH_ARGUMENTS;
}
commandResult_t CMD_DALY_BMS_Set_Pack_Voltage_Thresholds(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() == 2) {
		float vMin = Tokenizer_GetArgFloat(0);
		float vMax = Tokenizer_GetArgFloat(1);
        setMaxPackVoltage(vMax);
        setMinPackVoltage(vMin);
        g_firstIteration=true;
        return CMD_RES_OK;
	}
    return CMD_RES_NOT_ENOUGH_ARGUMENTS;
}
commandResult_t CMD_DALY_BMS_Set_Max_Charge_Current(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() == 1) {
		float current = Tokenizer_GetArgFloat(0);
        setChargeOvercurrent(current);
        g_firstIteration=true;
        return CMD_RES_OK;
	}
    return CMD_RES_NOT_ENOUGH_ARGUMENTS;
}
commandResult_t CMD_DALY_BMS_Set_Max_Discharge_Current(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() == 1) {
		float current = Tokenizer_GetArgFloat(0);
        setDischargeOvercurrent(current);
        g_firstIteration=true;
        return CMD_RES_OK;
	}
    return CMD_RES_NOT_ENOUGH_ARGUMENTS;
}
commandResult_t CMD_DALY_BMS_Set_Max_Voltage_Diff(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() == 1) {
		float voltage = Tokenizer_GetArgFloat(0);
        setMaxDiffVoltage(voltage);
        g_firstIteration=true;
        return CMD_RES_OK;
	}
    return CMD_RES_NOT_ENOUGH_ARGUMENTS;
}
commandResult_t CMD_DALY_BMS_Set_Charge_Temp_Thresholds(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() == 2) {
		float min = Tokenizer_GetArgFloat(0);
		float max = Tokenizer_GetArgFloat(1);
        setMinChargeTemp(min);
        setMaxChargeTemp(max);
        g_firstIteration=true;
        return CMD_RES_OK;
	}
    return CMD_RES_NOT_ENOUGH_ARGUMENTS;
}
commandResult_t CMD_DALY_BMS_Set_Discharge_Temp_Thresholds(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() == 2) {
		float min = Tokenizer_GetArgFloat(0);
		float max = Tokenizer_GetArgFloat(1);
        setMinDischargeTemp(min);
        setMaxDischargeTemp(max);
        g_firstIteration=true;
        return CMD_RES_OK;
	}
    return CMD_RES_NOT_ENOUGH_ARGUMENTS;
}
commandResult_t CMD_DALY_BMS_Set_Charge_Switch(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() == 1) {
		float value = Tokenizer_GetArgInteger(0);
        setChargeSwitch(value);
        return CMD_RES_OK;
	}
    return CMD_RES_NOT_ENOUGH_ARGUMENTS;
}
commandResult_t CMD_DALY_BMS_Set_Discharge_Switch(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() == 1) {
		float value = Tokenizer_GetArgInteger(0);
        setDischargeSwitch(value);
        return CMD_RES_OK;
	}
    return CMD_RES_NOT_ENOUGH_ARGUMENTS;
}
commandResult_t CMD_DALY_BMS_Set_Balance_Switch(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() == 1) {
		float value = Tokenizer_GetArgInteger(0);
        setBalanceSwitch(value);
        return CMD_RES_OK;
	}
    return CMD_RES_NOT_ENOUGH_ARGUMENTS;
}


void DALY_BMS_AddCommands(void) {
	CMD_RegisterCommand("DalyBms_Set_Cell_Voltage_Thresholds", CMD_DALY_BMS_Set_Cell_Voltage_Thresholds, NULL);
	CMD_RegisterCommand("DalyBms_Set_Pack_Voltage_Thresholds", CMD_DALY_BMS_Set_Pack_Voltage_Thresholds, NULL);
	CMD_RegisterCommand("DalyBms_Set_Max_Charge_Current", CMD_DALY_BMS_Set_Max_Charge_Current, NULL);
	CMD_RegisterCommand("DalyBms_Set_Max_Discharge_Current", CMD_DALY_BMS_Set_Max_Discharge_Current, NULL);
	CMD_RegisterCommand("DalyBms_Set_Max_Voltage_Diff", CMD_DALY_BMS_Set_Max_Voltage_Diff, NULL);
	CMD_RegisterCommand("DalyBms_Set_Charge_Temp_Thresholds", CMD_DALY_BMS_Set_Charge_Temp_Thresholds, NULL);
	CMD_RegisterCommand("DalyBms_Set_Discharge_Temp_Thresholds", CMD_DALY_BMS_Set_Discharge_Temp_Thresholds, NULL);
	CMD_RegisterCommand("DalyBms_Set_Charge_Switch", CMD_DALY_BMS_Set_Charge_Switch, NULL);
	CMD_RegisterCommand("DalyBms_Set_Discharge_Switch", CMD_DALY_BMS_Set_Discharge_Switch, NULL);
	CMD_RegisterCommand("DalyBms_Set_Balance_Switch", CMD_DALY_BMS_Set_Balance_Switch, NULL);
}
#endif
