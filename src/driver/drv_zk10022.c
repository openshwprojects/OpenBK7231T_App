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

#if ENABLE_DRIVER_ZK10022


static int g_baudRate = 115200;

static int writeRegister(int registerAddress,short value);


static SemaphoreHandle_t g_mutex = 0;

bool Mutex_Take(int del) {
	int taken;

	if (g_mutex == 0)
	{
		g_mutex = xSemaphoreCreateMutex();
	}
	taken = xSemaphoreTake(g_mutex, del);
	if (taken == pdTRUE) {
		return true;
	}
	return false;
}
void Mutex_Free() {
	xSemaphoreGive(g_mutex);
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

void readHoldingRegisters(){

	if(!Mutex_Take(10)){
		return;
    }
	unsigned char buffer[8];
	buffer[0] = 0x01;
	buffer[1] = 0x03;
	buffer[2] = 0x00;
	buffer[3] = 0x00;
	buffer[4] = 0x00;
	buffer[5] = 0x5C;
	uint16_t crc = MODBUS_CRC16(buffer, 6);

	buffer[6] = crc & 0xFF;
	buffer[7] = (crc >> 8) & 0xFF;

	for(int i = 0; i < 8; i++)
	{
		UART_SendByte(buffer[i]);
	}

	unsigned char receive_buffer[256];
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
    Mutex_Free();

	if(len==0){
        return;
    }

	if(receive_buffer[1]!=0x03){
		MQTT_PublishMain_StringInt("zk_10022_error_uart", receive_buffer[1], 0);
		return;
	}
	int registers [30];
	int register_count=receive_buffer[2]/2;
	int i=0;

	while(i<register_count){
		registers[i]=receive_buffer[3+i*2]*256+receive_buffer[4+i*2];
        i++;
	}
	float set_voltage=registers[0]*0.01;
	float set_current=registers[1]*0.01;
	float output_voltage=registers[2]*0.01;
	float output_current=registers[3]*0.01;
	float output_power=registers[4]*0.1;
	float input_voltage=registers[5]*0.01;
	float temperature=registers[0x0d]*0.1;
	bool protection_status = registers[0x10];
	bool constant_current_status = registers[0x11];
	bool switch_output = registers[0x12];

	float low_voltage_protection = registers[0x52] * 0.01
	float over_voltage_protection = registers[0x53] * 0.01
	float over_current_protection = registers[0x54] * 0.01
	float over_power_protection = registers[0x55] * 0.1
	float over_temperature_protection = registers[0x5C]*0.1

	#if ENABLE_MQTT
		MQTT_PublishMain_StringFloat("zk_10022_set_voltage", set_voltage,2, 0);
		MQTT_PublishMain_StringFloat("zk_10022_set_current", set_current,2, 0);
		MQTT_PublishMain_StringFloat("zk_10022_output_voltage", output_voltage,2, 0);
		MQTT_PublishMain_StringFloat("zk_10022_output_current", output_current,2, 0);
		MQTT_PublishMain_StringFloat("zk_10022_output_power", output_power,2, 0);
		MQTT_PublishMain_StringFloat("zk_10022_input_voltage", input_voltage,2, 0);
		MQTT_PublishMain_StringFloat("zk_10022_temperature", temperature,2, 0);
		MQTT_PublishMain_StringInt("zk_10022_protection_status", (int)protection_status, 0);
		MQTT_PublishMain_StringInt("zk_10022_constant_current_status", (int)constant_current_status, 0);
		MQTT_PublishMain_StringInt("zk_10022_switch_output", (int)switch_output, 0);
		MQTT_PublishMain_StringFloat("zk_10022_low_voltage_protection", low_voltage_protection,2, 0);
		MQTT_PublishMain_StringFloat("zk_10022_over_voltage_protection", over_voltage_protection,2, 0);
		MQTT_PublishMain_StringFloat("zk_10022_over_current_protection", over_current_protection,2, 0);
		MQTT_PublishMain_StringFloat("zk_10022_over_power_protection", over_power_protection,2, 0);
		MQTT_PublishMain_StringFloat("zk_10022_over_temperature_protection", over_temperature_protection,2, 0);
	#endif
}


void ZK10022_RunEverySecond()
{
		readHoldingRegisters();
}


// startDriver ZK10022 [baudrate] [hw flow control] [stop_bits] [data_width]

// backlog stopDriver ZK10022; startDriver ZK10022 115200 0 0 3
void ZK10022_Init()
{

	UART_InitUART(g_baudRate, 0,0,3, false);
	UART_InitReceiveRingBuffer(512);

}
void ZK10022_Deinit(){
}


commandResult_t CMD_ZK10022_Set_Voltage(const void* context, const char* cmd, const char* args, int cmdFlags) {

	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() == 1) {
		float voltage = Tokenizer_GetArgFloat(0);
		if(writeRegister(0,(short)(voltage*100))==0){
            return CMD_RES_OK;
        }
	}
    return CMD_RES_ERROR;
}
commandResult_t CMD_ZK10022_Set_Current(const void* context, const char* cmd, const char* args, int cmdFlags) {

	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() == 1) {
		float current = Tokenizer_GetArgFloat(0);
		if(writeRegister(1,(short)(current*100))==0){
            return CMD_RES_OK;
        }
    }
    return CMD_RES_ERROR;
}
commandResult_t CMD_ZK10022_Set_LowVoltageProtection(const void* context, const char* cmd, const char* args, int cmdFlags) {

	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() == 1) {
		float current = Tokenizer_GetArgFloat(0);
		if(writeRegister(0x52,(short)(current*100))==0){
            return CMD_RES_OK;
        }
    }
    return CMD_RES_ERROR;
}
commandResult_t CMD_ZK10022_Set_OverVoltageProtection(const void* context, const char* cmd, const char* args, int cmdFlags) {

	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() == 1) {
		float current = Tokenizer_GetArgFloat(0);
		if(writeRegister(0x53,(short)(current*100))==0){
            return CMD_RES_OK;
        }
    }
    return CMD_RES_ERROR;
}
commandResult_t CMD_ZK10022_Set_OverCurrentProtection(const void* context, const char* cmd, const char* args, int cmdFlags) {

	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() == 1) {
		float current = Tokenizer_GetArgFloat(0);
		if(writeRegister(0x54,(short)(current*100))==0){
            return CMD_RES_OK;
        }
    }
    return CMD_RES_ERROR;
}
commandResult_t CMD_ZK10022_Set_OverPowerProtection(const void* context, const char* cmd, const char* args, int cmdFlags) {

	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() == 1) {
		float current = Tokenizer_GetArgFloat(0);
		if(writeRegister(0x55,(short)(current*10))==0){
            return CMD_RES_OK;
        }
    }
    return CMD_RES_ERROR;
}
commandResult_t CMD_ZK10022_Set_OverTemperatureProtection(const void* context, const char* cmd, const char* args, int cmdFlags) {

	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() == 1) {
		float current = Tokenizer_GetArgFloat(0);
		if(writeRegister(0x5C,(short)(current*10))==0){
            return CMD_RES_OK;
        }
    }
    return CMD_RES_ERROR;
}
commandResult_t CMD_ZK10022_Set_Switch(const void* context, const char* cmd, const char* args, int cmdFlags) {

	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() == 1) {
		float current = Tokenizer_GetArgInteger(0);
		if(writeRegister(0x12,(int)(current*100))==0){
            return CMD_RES_OK;
        }
    }
    return CMD_RES_ERROR;
}

int writeRegister(int registerAddress,short value){

	if(!Mutex_Take(500)){
		ADDLOG_ERROR(LOG_FEATURE_DRV, "Locking Mutex failed\n");
		return 1;
    }
	unsigned char buffer[8];
	buffer[0] = 0x01;
	buffer[1] = 0x06;

	buffer[2] = registerAddress>>8 & 0xFF;
	buffer[3] = registerAddress & 0xFF;

	buffer[4] = (byte)(value >> 8 & 0xFF) ;
	buffer[5] = (byte)(value & 0xFF) ;
	uint16_t crc = MODBUS_CRC16(buffer, 6);

	buffer[6] = crc & 0xFF;
	buffer[7] = (crc >> 8) & 0xFF;


	for(int i = 0; i < 8; i++)
	{
		UART_SendByte(buffer[i]);
	}

	unsigned char receive_buffer[256];
	int len = UART_GetDataSize();

	int delay=0;
	while(len == 0 && delay < 250)
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
    Mutex_Free();
	if(receive_buffer[1]!=0x06){
		ADDLOG_ERROR(LOG_FEATURE_DRV, "Error UART\n");
		MQTT_PublishMain_StringInt("zk_10022_error_uart_write", receive_buffer[1], 0);
		return 1;
	}
    return 0;
}


void ZK10022_AddCommands(void) {
	//cmddetail:{"name":"BL0942opts","args":"opts",
	//cmddetail:"descr":"BL0942opts 0= default mode (as set in config Flag 26), 3= two BL0942 on both UARTs (bit0 BL0942 on UART1, bit1 BL0942 on UART2)",
	//cmddetail:"fn":"CMD_BL0942opts","file":"driver/drv_bl0942.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("ZK10022_Set_Voltage", CMD_ZK10022_Set_Voltage, NULL);
	CMD_RegisterCommand("ZK10022_Set_Current", CMD_ZK10022_Set_Current, NULL);
	CMD_RegisterCommand("ZK10022_Set_Switch", CMD_ZK10022_Set_Switch, NULL);
	CMD_RegisterCommand("ZK10022_Set_LowVoltageProtection", CMD_ZK10022_Set_LowVoltageProtection, NULL);
	CMD_RegisterCommand("ZK10022_Set_OverVoltageProtection", CMD_ZK10022_Set_OverVoltageProtection, NULL);
	CMD_RegisterCommand("ZK10022_Set_OverCurrentProtection", CMD_ZK10022_Set_OverCurrentProtection, NULL);
	CMD_RegisterCommand("ZK10022_Set_OverPowerProtection", CMD_ZK10022_Set_OverPowerProtection, NULL);
	CMD_RegisterCommand("ZK10022_Set_OverTemperatureProtection", CMD_ZK10022_Set_OverTemperatureProtection, NULL);
}
#endif
