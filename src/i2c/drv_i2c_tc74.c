
//
//	TC74 - I2C temperature sensor - read only single integer value, temperature in C
//
/*
// TC74 A0 (address type 0)
// setChannelType 5 temperature
// addI2CDevice_TC74 I2C1 0x48 5
// TC74 A2 (address type 2)
// setChannelType 6 temperature
// addI2CDevice_TC74 I2C1 0x4A 6
// TC74 A5 (address type 5)
setChannelType 6 temperature
addI2CDevice_TC74 Soft 0x4D 6
*/
#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "drv_i2c_local.h"

typedef struct i2cDevice_TC74_s {
	i2cDevice_t base;
	// private TC74 variables
	// Our channel index to save the result temp
	int targetChannel;
} i2cDevice_TC74_t;

int DRV_I2C_TC74_readTemperature(int dev_adr, int busID)
{
	byte temp;

	//addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_TC74_readTemperature: called for addr %i\n", dev_adr);

	DRV_I2C_Begin(dev_adr,busID);
	DRV_I2C_Write(0,0x00);
	DRV_I2C_Read(0x00,&temp);
	DRV_I2C_Close();

	addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"TC74 reads %i", temp);

	return temp;

}
void DRV_I2C_TC74_RunDevice(i2cDevice_t *dev)
{
	i2cDevice_TC74_t *tc74;
	byte temp;

	tc74 = (i2cDevice_TC74_t*)dev;

	temp = DRV_I2C_TC74_readTemperature(tc74->base.addr, tc74->base.busType);

	CHANNEL_Set(tc74->targetChannel, temp, 0);
}
void DRV_I2C_AddDevice_TC74_Internal(int busType, int address, int targetChannel) {
	i2cDevice_TC74_t *dev;

	dev = malloc(sizeof(i2cDevice_TC74_t));

	dev->base.addr = address;
	dev->base.busType = busType;
	dev->base.type = I2CDEV_TC74;
	dev->base.next = 0;
	dev->base.runFrame = DRV_I2C_TC74_RunDevice;
	dev->base.channelChange = 0;
	dev->targetChannel = targetChannel;

	DRV_I2C_AddNextDevice((i2cDevice_t*)dev);
}
commandResult_t DRV_I2C_AddDevice_TC74(const void *context, const char *cmd, const char *args, int cmdFlags) {
	const char *i2cModuleStr;
	int address;
	int targetChannel;
	i2cBusType_t busType;

	Tokenizer_TokenizeString(args, 0);
	i2cModuleStr = Tokenizer_GetArg(0);
	address = Tokenizer_GetArgInteger(1);
	targetChannel = Tokenizer_GetArgInteger(2);

	busType = DRV_I2C_ParseBusType(i2cModuleStr);

	if (DRV_I2C_FindDevice(busType, address)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_I2C, "DRV_I2C_AddDevice_TC74: there is already some device on this bus with such addr\n");
		return CMD_RES_BAD_ARGUMENT;
	}

	addLogAdv(LOG_INFO, LOG_FEATURE_I2C, "DRV_I2C_AddDevice_TC74: module %s, address %i, target %i\n", i2cModuleStr, address, targetChannel);

	DRV_I2C_AddDevice_TC74_Internal(busType, address, targetChannel);

	return CMD_RES_OK;
}


void DRV_I2C_TC74_PreInit() {

	//cmddetail:{"name":"addI2CDevice_TC74","args":"",
	//cmddetail:"descr":"Adds a new I2C device - TC74",
	//cmddetail:"fn":"DRV_I2C_AddDevice_TC74","file":"i2c/drv_i2c_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("addI2CDevice_TC74", DRV_I2C_AddDevice_TC74, NULL);

}