#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "drv_i2c_local.h"
// addresses, banks, etc, defines
#include "drv_i2c_ads1115.h"

#define CONFIG_REGISTER 0x01
#define CONVERSION_REGISTER 0x00
// default config for 16-bit resolution, single-ended, channels AIN0, AIN1, AIN2, AIN3
#define CONFIG_DEFAULT 0xC183  // 16-bit, AIN0, single-ended


typedef struct i2cDevice_ADS1115_s {
	i2cDevice_t base;
	byte channels[4];
} i2cDevice_ADS1115_t;



/*
// set SoftSDA and SoftSCL pins
startDriver I2C
// use adr here from scanI2C, I'm assuming 0x48
addI2CDevice_ADS1115 Soft 0x48 0 1 2 3
// Btw, if you are only using input 0, you can also do
// addI2CDevice_ADS1115 Soft 0x48 0
// Others will not be read.
// You can also use 0xff to mark skip input
// addI2CDevice_ADS1115 Soft 0x48 0 0xff 2 3

*/
void ADS1115_WriteConfig(i2cDevice_ADS1115_t *ads, uint16_t config) {
	byte payload[3];
	payload[0] = CONFIG_REGISTER;
	payload[1] = (config >> 8) & 0xFF;
	payload[2] = config & 0xFF;
	DRV_I2C_Begin(ads->base.addr, ads->base.busType);
	DRV_I2C_WriteBytesSingle(payload, 3);
	DRV_I2C_Close();
}
int ADS1115_ReadChannel(i2cDevice_ADS1115_t *ads, int channel)
{
	uint16_t config = CONFIG_DEFAULT | (channel << 12);
	ADS1115_WriteConfig(ads, config);

	delay_ms(8);

	byte dat[2];
	DRV_I2C_Begin(ads->base.addr, ads->base.busType);
	DRV_I2C_ReadBytes(CONVERSION_REGISTER, dat, 2);
	DRV_I2C_Close();
	int res = (dat[0] << 8) | dat[1];
	// DeDaMrAz hack?
	if (res > 60000) {
		res = 0;
	}

	addLogAdv(LOG_INFO, LOG_FEATURE_I2C, "ADS adr %i AN%i is %i", ads->base.addr, channel, (int)res);
	return res;
}
void DRV_I2C_ADS1115_RunDevice(i2cDevice_t *dev)
{
	i2cDevice_ADS1115_t *ads;

	ads = (i2cDevice_ADS1115_t*)dev;

	for (int i = 0; i < 4; i++) {
		if (ads->channels[i] != 0xff) {
			int res = ADS1115_ReadChannel(dev, i);
			CHANNEL_Set(ads->channels[i], res, 0);
		}
	}
}


void DRV_I2C_AddDevice_ADS1115_Internal(int busType, int address, byte channels[4]) {
	i2cDevice_ADS1115_t *dev;

	dev = malloc(sizeof(i2cDevice_ADS1115_t));

	dev->base.addr = address;
	dev->base.busType = busType;
	dev->base.type = I2CDEV_ADS1115;
	dev->base.next = 0;
	dev->base.runFrame = DRV_I2C_ADS1115_RunDevice;
	dev->base.channelChange = 0;
	memcpy(dev->channels, channels, sizeof(dev->channels));

	DRV_I2C_AddNextDevice((i2cDevice_t*)dev);
}
//
commandResult_t DRV_I2C_AddDevice_ADS1115(const void *context, const char *cmd, const char *args, int cmdFlags) {
	const char *i2cModuleStr;
	int address;
	i2cBusType_t busType;
	byte channels[4];

	Tokenizer_TokenizeString(args, 0);
	i2cModuleStr = Tokenizer_GetArg(0);
	address = Tokenizer_GetArgInteger(1);

	busType = DRV_I2C_ParseBusType(i2cModuleStr);

	if (DRV_I2C_FindDevice(busType, address)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_I2C, "DRV_I2C_AddDevice_ADS1115: there is already some device on this bus with such addr\n");
		return CMD_RES_BAD_ARGUMENT;
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_I2C, "DRV_I2C_AddDevice_ADS1115: module %s, address %i\n", i2cModuleStr, address);

	for (int i = 0; i < 4; i++) {
		channels[i] = Tokenizer_GetArgIntegerDefault(2 + i, 0xff);
	}

	DRV_I2C_AddDevice_ADS1115_Internal(busType, address, channels);

	return CMD_RES_OK;
}
void DRV_I2C_ADS1115_PreInit() {

	//cmddetail:{"name":"addI2CDevice_ADS1115","args":"",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"NULL);","file":"i2c/drv_i2c_ads1115.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("addI2CDevice_ADS1115", DRV_I2C_AddDevice_ADS1115, NULL);
}

