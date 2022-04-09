#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../new_cmd.h"
#include "../logging/logging.h"
#include "drv_i2c_local.h"


#if PLATFORM_BK7231T

#include "i2c_pub.h"
I2C_OP_ST i2c_operater;
DD_HANDLE i2c_hdl;

void DRV_I2C_Write(UINT8 addr, UINT8 data)
{
	char buff = (char)data;
    
    i2c_operater.op_addr = addr;
    ddev_write(i2c_hdl, &buff, 1, (UINT32)&i2c_operater);
}

void DRV_I2C_Read(UINT8 addr, UINT8 *data)
{   
    i2c_operater.op_addr = addr;
    ddev_read(i2c_hdl, (char*)data, 1, (UINT32)&i2c_operater);
}
int DRV_I2C_Begin(int dev_adr, int busID) {

    uint32_t status;
	uint32_t oflag;
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

void DRV_I2C_Write(UINT8 addr, UINT8 data)
{
}

void DRV_I2C_Read(UINT8 addr, UINT8 *data)
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
void DRV_I2C_AddDevice_TC74_Internal(int busType,int address, int targetChannel) {
	i2cDevice_TC74_t *dev;

	dev = malloc(sizeof(i2cDevice_TC74_t));

	dev->base.addr = address;
	dev->base.busType = busType;
	dev->base.type = I2CDEV_TC74;
	dev->base.next = 0;
	dev->targetChannel = targetChannel;

	DRV_I2C_AddNextDevice(dev);
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

	//addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_AddDevice_TC74: module %s, address %i, target %i\n", i2cModuleStr, address, targetChannel);

	busType = DRV_I2C_ParseBusType(i2cModuleStr);

	DRV_I2C_AddDevice_TC74_Internal(busType,address,targetChannel);

	return 1;
}
int DRV_I2C_AddDevice_MCP23017(const void *context, const char *cmd, const char *args) {
	const char *i2cModuleStr;
	int address;
	int targetChannel;
	i2cBusType_t busType;

	Tokenizer_TokenizeString(args);
	i2cModuleStr = Tokenizer_GetArg(0);
	address = Tokenizer_GetArgInteger(1);

	addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"addI2CDevice_MCP23017: module %s, address %i\n", i2cModuleStr, address);

	busType = DRV_I2C_ParseBusType(i2cModuleStr);

//	DRV_I2C_AddDevice_MCP23017_Internal(busType,address,targetChannel);

	return 1;
}
// TC74 A0 (address type 0)
// setChannelType 5 temperature
// addI2CDevice_TC74 I2C1 0x48 5 
// TC74 A2 (address type 2)
// setChannelType 6 temperature
// addI2CDevice_TC74 I2C1 0x4A 6 

// MCP23017 with A0=1, A1=1, A2=1
// addI2CDevice_MCP23017 I2C1 0x4E 6 
void DRV_I2C_Init()
{
	CMD_RegisterCommand("addI2CDevice_TC74","",DRV_I2C_AddDevice_TC74, "Adds a new I2C device", NULL);
	CMD_RegisterCommand("addI2CDevice_MCP23017","",DRV_I2C_AddDevice_MCP23017, "Adds a new I2C device", NULL);

}
void DRC_I2C_RunDevice(i2cDevice_t *dev)
{
	switch(dev->type)
	{
	case I2CDEV_TC74:
		DRV_I2C_TC74_RunDevice(dev);
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
