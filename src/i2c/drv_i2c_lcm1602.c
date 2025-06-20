#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "drv_i2c_local.h"



commandResult_t DRV_I2C_AddDevice_LCM1602(const void *context, const char *cmd, const char *args, int cmdFlags) {
	const char *i2cModuleStr;
	int address;
	i2cBusType_t busType;

	Tokenizer_TokenizeString(args, 0);
	i2cModuleStr = Tokenizer_GetArg(0);
	address = Tokenizer_GetArgInteger(1);

	busType = DRV_I2C_ParseBusType(i2cModuleStr);

	if (DRV_I2C_FindDevice(busType, address)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_I2C, "DRV_I2C_AddDevice_LCM1602: there is already some device on this bus with such addr\n");
		return CMD_RES_BAD_ARGUMENT;
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_I2C, "DRV_I2C_AddDevice_LCM1602: module %s, address %i\n", i2cModuleStr, address);

	// DRV_I2C_AddDevice_LCM1602_Internal(busType,address);

	return CMD_RES_OK;
}
void DRV_I2C_LCM1602_PreInit() {
	//cmddetail:{"name":"addI2CDevice_LCM1602","args":"",
	//cmddetail:"descr":"Adds a new I2C device - LCM1602",
	//cmddetail:"fn":"DRV_I2C_AddDevice_LCM1602","file":"i2c/drv_i2c_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("addI2CDevice_LCM1602", DRV_I2C_AddDevice_LCM1602, NULL);


}
