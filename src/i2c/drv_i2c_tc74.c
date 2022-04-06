#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../new_cmd.h"
#include "../logging/logging.h"
#include "drv_i2c_local.h"

int DRV_I2C_TC74_readTemperature(int dev_adr)
{
	byte temp;

	addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_TC74_readTemperature: called for addr %i\n", dev_adr);

	DRV_I2C_Begin(dev_adr);
	DRV_I2C_Write(0,0x00);
	DRV_I2C_Read(0x00,&temp);
	DRV_I2C_Close();

	addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_TC74_readTemperature: result is %i\n", temp);

	return temp;

}
void DRV_I2C_TC74_RunDevice(i2cDevice_t *dev)
{
	i2cDevice_TC74_t *tc74;
	byte temp;

	tc74 = (i2cDevice_TC74_t*)dev;

	temp = DRV_I2C_TC74_readTemperature(tc74->base.addr);

	CHANNEL_Set(tc74->targetChannel, temp, 0);
}