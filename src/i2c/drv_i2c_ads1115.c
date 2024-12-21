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
	addLogAdv(LOG_INFO, LOG_FEATURE_I2C, "ADS ch %i is %i", channel, (int)res);


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


