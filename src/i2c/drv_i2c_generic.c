#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "drv_i2c_local.h"

int DRV_I2C_GENERIC_read(int dev_adr, int busID)
{
	byte temp[4];

	//addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_TC74_readTemperature: called for addr %i\n", dev_adr);

	DRV_I2C_Begin(dev_adr,busID);
	DRV_I2C_Write(0,0x00);
	DRV_I2C_ReadBytes(0x00,temp,4);
	DRV_I2C_Close();

for (int i = 0; i<4; i++)
	addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"Generic reads %i - %i", i, temp[i]);

	return temp[0];

}
void DRV_I2C_GENERIC_RunDevice(i2cDevice_t *dev)
{
	i2cDevice_TC74_t *tc74;
	byte temp;

	tc74 = (i2cDevice_TC74_t*)dev;

	temp = DRV_I2C_GENERIC_read(tc74->base.addr, tc74->base.busType);

	CHANNEL_Set(tc74->targetChannel, temp, 0);
}