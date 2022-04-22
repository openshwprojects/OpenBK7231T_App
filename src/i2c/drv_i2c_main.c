#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "drv_i2c_local.h"
#include "drv_i2c_public.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"


#if PLATFORM_BK7231T

#include "i2c_pub.h"
I2C_OP_ST i2c_operater;
DD_HANDLE i2c_hdl;

void DRV_I2C_Write(byte addr, byte data)
{
	char buff = (char)data;
    
    i2c_operater.op_addr = addr;
    ddev_write(i2c_hdl, &buff, 1, (UINT32)&i2c_operater);
}
void DRV_I2C_WriteBytes(byte addr, byte *data, int len) {

    
    i2c_operater.op_addr = addr;
    ddev_write(i2c_hdl, (char*)data, len, (UINT32)&i2c_operater);
}
//void DRV_I2C_WriteSingle(byte addr)
//{
//	char data;
//	char buff = (char)data;
//    
//    i2c_operater.op_addr = addr;
//    ddev_write(i2c_hdl, &buff, 0, (UINT32)&i2c_operater);
//}


void DRV_I2C_Read(byte addr, byte *data)
{   
    i2c_operater.op_addr = addr;
    ddev_read(i2c_hdl, (char*)data, 1, (UINT32)&i2c_operater);
}
int DRV_I2C_Begin(int dev_adr, int busID) {

    UINT32 status;
	UINT32 oflag;
    oflag = I2C_DEF_DIV;

	if(busID == I2C_BUS_I2C1) {
		i2c_hdl = ddev_open("i2c1", &status, oflag);
	} else if(busID == I2C_BUS_I2C2) {
		i2c_hdl = ddev_open("i2c2", &status, oflag);
	} else {
		addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_Begin bus type %i not supported!\n",busID);
		return 1;
	}
    if(DD_HANDLE_UNVALID == i2c_hdl){
		addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_Begin ddev_open failed, status %i!\n",status);
		return 1;
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_Begin ddev_open OK, adr %i!\n",dev_adr);

    i2c_operater.salve_id = dev_adr;

	return 0;
}
void DRV_I2C_Close() {

	ddev_close(i2c_hdl);
}
#else

void DRV_I2C_Write(byte addr, byte data)
{
}
void DRV_I2C_WriteBytes(byte addr, byte *data, int len) {

}

void DRV_I2C_Read(byte addr, byte *data)
{   
}
int DRV_I2C_Begin(int dev_adr, int busID) {

	return 1; // error
}
void DRV_I2C_Close() {

}
#endif


i2cDevice_t *g_i2c_devices = 0;

i2cBusType_t DRV_I2C_ParseBusType(const char *s) {
	if(!stricmp(s,"I2C1"))
		return I2C_BUS_I2C1;
	if(!stricmp(s,"I2C2"))
		return I2C_BUS_I2C2;
	return I2C_BUS_ERROR;
}
void DRV_I2C_AddNextDevice(i2cDevice_t *t) {
	t->next = g_i2c_devices;
	g_i2c_devices = t;
}
void DRV_I2C_AddDevice_PCF8574_Internal(int busType,int address, byte lcd_cols, byte lcd_rows, byte charsize) {
	i2cDevice_PCF8574_t *dev;

	dev = malloc(sizeof(i2cDevice_PCF8574_t));

	dev->base.addr = address;
	dev->base.busType = busType;
	dev->base.type = I2CDEV_LCD_PCF8574;
	dev->base.next = 0;
	dev->lcd_cols = lcd_cols;
	dev->lcd_rows = lcd_rows;
	dev->charsize = charsize;
	dev->LCD_BL_Status = 1;

	DRV_I2C_AddNextDevice((i2cDevice_t*)dev);
}
void DRV_I2C_AddDevice_MCP23017_Internal(int busType,int address) {
	i2cDevice_MCP23017_t *dev;

	dev = malloc(sizeof(i2cDevice_MCP23017_t));

	dev->base.addr = address;
	dev->base.busType = busType;
	dev->base.type = I2CDEV_MCP23017;
	dev->base.next = 0;
	memset(dev->pinMapping,0xff,sizeof(dev->pinMapping));

	DRV_I2C_AddNextDevice((i2cDevice_t*)dev);
}
i2cDevice_t *DRV_I2C_FindDevice(int busType,int address) {
	i2cDevice_t *dev;

	dev = g_i2c_devices;
	while(dev!=0) {
		if(dev->addr == address && dev->busType == busType)
			return dev;
		dev = dev->next;
	}
	return 0;
}
i2cDevice_t *DRV_I2C_FindDeviceExt(int busType,int address, int devType) {
	i2cDevice_t *dev;

	dev = g_i2c_devices;
	while(dev!=0) {
		if(dev->addr == address && dev->busType == busType && dev->type == devType)
			return dev;
		dev = dev->next;
	}
	return 0;
}
void DRV_I2C_AddDevice_TC74_Internal(int busType,int address, int targetChannel) {
	i2cDevice_TC74_t *dev;

	dev = malloc(sizeof(i2cDevice_TC74_t));

	dev->base.addr = address;
	dev->base.busType = busType;
	dev->base.type = I2CDEV_TC74;
	dev->base.next = 0;
	dev->targetChannel = targetChannel;

	DRV_I2C_AddNextDevice((i2cDevice_t*)dev);
}
int DRV_I2C_AddDevice_TC74(const void *context, const char *cmd, const char *args) {
	const char *i2cModuleStr;
	int address;
	int targetChannel;
	i2cBusType_t busType;

	Tokenizer_TokenizeString(args);
	i2cModuleStr = Tokenizer_GetArg(0);
	address = Tokenizer_GetArgInteger(1);
	targetChannel = Tokenizer_GetArgInteger(2);

	busType = DRV_I2C_ParseBusType(i2cModuleStr);

	if(DRV_I2C_FindDevice(busType,address)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_AddDevice_TC74: there is already some device on this bus with such addr\n");
		return 1;
	}

	addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_AddDevice_TC74: module %s, address %i, target %i\n", i2cModuleStr, address, targetChannel);

	DRV_I2C_AddDevice_TC74_Internal(busType,address,targetChannel);

	return 1;
}
int DRV_I2C_AddDevice_MCP23017(const void *context, const char *cmd, const char *args) {
	const char *i2cModuleStr;
	int address;
	i2cBusType_t busType;

	Tokenizer_TokenizeString(args);
	i2cModuleStr = Tokenizer_GetArg(0);
	address = Tokenizer_GetArgInteger(1);

	busType = DRV_I2C_ParseBusType(i2cModuleStr);

	if(DRV_I2C_FindDevice(busType,address)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_AddDevice_MCP23017: there is already some device on this bus with such addr\n");
		return 1;
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_AddDevice_MCP23017: module %s, address %i\n", i2cModuleStr, address);


	DRV_I2C_AddDevice_MCP23017_Internal(busType,address);

	return 1;
}

// 
int DRV_I2C_AddDevice_PCF8574(const void *context, const char *cmd, const char *args) {
	const char *i2cModuleStr;
	int address;
	i2cBusType_t busType;
	byte lcd_cols, lcd_rows, charsize;

	Tokenizer_TokenizeString(args);
	i2cModuleStr = Tokenizer_GetArg(0);
	address = Tokenizer_GetArgInteger(1);

	busType = DRV_I2C_ParseBusType(i2cModuleStr);

	if(DRV_I2C_FindDevice(busType,address)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_AddDevice_PCF8574: there is already some device on this bus with such addr\n");
		return 1;
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_AddDevice_PCF8574: module %s, address %i\n", i2cModuleStr, address);

	lcd_cols = Tokenizer_GetArgInteger(2);
	lcd_rows = Tokenizer_GetArgInteger(3);
	charsize = Tokenizer_GetArgInteger(4);
	// DRV_I2C_AddDevice_LCM1602_Internal(busType,address);

	DRV_I2C_AddDevice_PCF8574_Internal(busType,address,lcd_cols,lcd_rows,charsize);

	return 1;
}

int DRV_I2C_AddDevice_LCM1602(const void *context, const char *cmd, const char *args) {
	const char *i2cModuleStr;
	int address;
	i2cBusType_t busType;

	Tokenizer_TokenizeString(args);
	i2cModuleStr = Tokenizer_GetArg(0);
	address = Tokenizer_GetArgInteger(1);

	busType = DRV_I2C_ParseBusType(i2cModuleStr);

	if(DRV_I2C_FindDevice(busType,address)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_AddDevice_LCM1602: there is already some device on this bus with such addr\n");
		return 1;
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_AddDevice_LCM1602: module %s, address %i\n", i2cModuleStr, address);

	// DRV_I2C_AddDevice_LCM1602_Internal(busType,address);

	return 1;
}
//
//	TC74 - I2C temperature sensor - read only single integer value, temperature in C
//
// TC74 A0 (address type 0)
// setChannelType 5 temperature
// addI2CDevice_TC74 I2C1 0x48 5 
// TC74 A2 (address type 2)
// setChannelType 6 temperature
// addI2CDevice_TC74 I2C1 0x4A 6 

//
//	MCP23017 - I2C 16 bit port expander - both inputs and outputs, right now we use it only as outputs
//
// MCP23017 with A0=1, A1=1, A2=1
// addI2CDevice_MCP23017 I2C1 0x27

// maps channels 5 and 6 to GPIO A7 and A6 of MCP23017
// backlog setChannelType 5 toggle; setChannelType 6 toggle; addI2CDevice_MCP23017 I2C1 0x27; MCP23017_MapPinToChannel I2C1 0x27 7 5; MCP23017_MapPinToChannel I2C1 0x27 6 6

// maps channels 5 6 7 8 etc
// backlog setChannelType 5 toggle; setChannelType 6 toggle; setChannelType 7 toggle; setChannelType 8 toggle; setChannelType 9 toggle; setChannelType 10 toggle; setChannelType 11 toggle; addI2CDevice_MCP23017 I2C1 0x27; MCP23017_MapPinToChannel I2C1 0x27 7 5; MCP23017_MapPinToChannel I2C1 0x27 6 6; MCP23017_MapPinToChannel I2C1 0x27 5 7; MCP23017_MapPinToChannel I2C1 0x27 4 8; MCP23017_MapPinToChannel I2C1 0x27 3 9; MCP23017_MapPinToChannel I2C1 0x27 2 10; MCP23017_MapPinToChannel I2C1 0x27 1 11

void DRV_I2C_Init()
{
	CMD_RegisterCommand("addI2CDevice_TC74","",DRV_I2C_AddDevice_TC74, "Adds a new I2C device", NULL);
	CMD_RegisterCommand("addI2CDevice_MCP23017","",DRV_I2C_AddDevice_MCP23017, "Adds a new I2C device", NULL);
	CMD_RegisterCommand("addI2CDevice_LCM1602","",DRV_I2C_AddDevice_LCM1602, "Adds a new I2C device", NULL);
	CMD_RegisterCommand("addI2CDevice_LCD_PCF8574","",DRV_I2C_AddDevice_PCF8574, "Adds a new I2C device", NULL);
	CMD_RegisterCommand("MCP23017_MapPinToChannel","",DRV_I2C_MCP23017_MapPinToChannel, "Adds a new I2C device", NULL);
 
}
void DRC_I2C_RunDevice(i2cDevice_t *dev)
{
	switch(dev->type)
	{
	case I2CDEV_TC74:
		DRV_I2C_TC74_RunDevice(dev);
		break;
	case I2CDEV_MCP23017:
		DRV_I2C_MCP23017_RunDevice(dev);
		break;
	case I2CDEV_LCD_PCF8574:
		DRV_I2C_LCD_PCF8574_RunDevice(dev);
		break;
	default:
		addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRC_I2C_RunDevice: error, device of type %i at adr %i not handled\n", dev->type, dev->addr);
		break;


	}
}
void DRV_I2C_EverySecond()
{
	i2cDevice_t *cur;

	cur = g_i2c_devices;
	while(cur) {
		//addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_EverySecond: going to run device of type %i with addr %i\n", cur->type, cur->addr);
		DRC_I2C_RunDevice(cur);
		cur = cur->next;
	}
}
void I2C_OnChannelChanged_Device(i2cDevice_t *dev, int channel, int iVal)
{
	switch(dev->type)
	{
	case I2CDEV_TC74:
		// not needed
		break;
	case I2CDEV_MCP23017:
		DRV_I2C_MCP23017_OnChannelChanged(dev, channel, iVal);
		break;
	case I2CDEV_LCD_PCF8574:
		// not needed
		break;
	default:
		addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"I2C_OnChannelChanged: error, device of type %i at adr %i not handled\n", dev->type, dev->addr);
		break;


	}
}
void I2C_OnChannelChanged(int channel,int iVal) {
	i2cDevice_t *cur;

	cur = g_i2c_devices;
	while(cur) {
		//addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_EverySecond: going to run device of type %i with addr %i\n", cur->type, cur->addr);
		I2C_OnChannelChanged_Device(cur,channel,iVal);
		cur = cur->next;
	}
}


