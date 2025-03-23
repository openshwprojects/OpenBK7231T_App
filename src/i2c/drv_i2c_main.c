#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../driver/drv_local.h"
#include "drv_i2c_local.h"
#include "drv_i2c_public.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../hal/hal_pins.h"




#if PLATFORM_BK7231T

#include "i2c_pub.h"
static I2C_OP_ST i2c_operater;
static DD_HANDLE i2c_hdl;

#endif

static int current_bus;
static int tg_addr;
static softI2C_t g_softI2C;

void DRV_I2C_Write(byte addr, byte data)
{
	if (current_bus == I2C_BUS_SOFT) {
		Soft_I2C_Start(&g_softI2C, (tg_addr << 1) + 0);
		Soft_I2C_WriteByte(&g_softI2C, addr);
		Soft_I2C_Stop(&g_softI2C);
		Soft_I2C_Start(&g_softI2C, (tg_addr << 1) + 0);
		Soft_I2C_WriteByte(&g_softI2C, data);
		Soft_I2C_Stop(&g_softI2C);
		return;
	}
#if PLATFORM_BK7231T
    i2c_operater.op_addr = addr;
    ddev_write(i2c_hdl, (char*)&data, 1, (UINT32)&i2c_operater);
#endif
}
void DRV_I2C_WriteBytesSingle(byte *data, int len) {
	if (current_bus == I2C_BUS_SOFT) {
		Soft_I2C_Start(&g_softI2C, (tg_addr << 1) + 0);
		for (int i = 0; i < len; i++) {
			Soft_I2C_WriteByte(&g_softI2C, data[i]);
		}
		Soft_I2C_Stop(&g_softI2C);
		return;
	}
	// TODO - how to do it in BK I2C?
}
void DRV_I2C_WriteBytes(byte addr, byte *data, int len) {
	if (current_bus == I2C_BUS_SOFT) {
		Soft_I2C_Start(&g_softI2C, (tg_addr << 1) +0);
		Soft_I2C_WriteByte(&g_softI2C, addr);
		Soft_I2C_Stop(&g_softI2C);
		Soft_I2C_Start(&g_softI2C, (tg_addr << 1) + 0);
		for (int i = 0; i < len; i++) {
			Soft_I2C_WriteByte(&g_softI2C, data[i]);
		}
		Soft_I2C_Stop(&g_softI2C);
		return;
	}
#if PLATFORM_BK7231T
    i2c_operater.op_addr = addr;
    ddev_write(i2c_hdl, (char*)data, len, (UINT32)&i2c_operater);
#endif
}
void DRV_I2C_Read(byte addr, byte *data)
{
	if (current_bus == I2C_BUS_SOFT) {
		Soft_I2C_Start(&g_softI2C, (tg_addr << 1) + 0);
		Soft_I2C_WriteByte(&g_softI2C, addr);
		Soft_I2C_Stop(&g_softI2C);
		Soft_I2C_Start(&g_softI2C, (tg_addr << 1) + 1);
		*data = Soft_I2C_ReadByte(&g_softI2C, false);
		Soft_I2C_Stop(&g_softI2C);
		return;
	}
#if PLATFORM_BK7231T
    i2c_operater.op_addr = addr;
    ddev_read(i2c_hdl, (char*)data, 1, (UINT32)&i2c_operater);
#endif
}
void DRV_I2C_ReadBytes(byte addr, byte *data, int size)
{
	if (current_bus == I2C_BUS_SOFT) {
		Soft_I2C_Start(&g_softI2C, (tg_addr << 1) + 0);
		Soft_I2C_WriteByte(&g_softI2C, addr);
		Soft_I2C_Stop(&g_softI2C);
		Soft_I2C_Start(&g_softI2C, (tg_addr << 1) + 1);
		for (int i = 0; i < size; i++) {
			data[i] = Soft_I2C_ReadByte(&g_softI2C, !(i < size-1));
		}
		Soft_I2C_Stop(&g_softI2C);
		return;
}
#if PLATFORM_BK7231T
	i2c_operater.op_addr = addr;
	ddev_read(i2c_hdl, (char*)data, size, (UINT32)&i2c_operater);
#endif
}
int DRV_I2C_Begin(int dev_adr, int busID) {

#if PLATFORM_BK7231T
    UINT32 status;
	UINT32 oflag;
    oflag = I2C_DEF_DIV;
#endif

	current_bus = busID;
	tg_addr = dev_adr;

	if (busID == I2C_BUS_SOFT) {
		g_softI2C.pin_clk = PIN_FindPinIndexForRole(IOR_SOFT_SCL, g_softI2C.pin_clk);
		g_softI2C.pin_data = PIN_FindPinIndexForRole(IOR_SOFT_SDA, g_softI2C.pin_data);
		Soft_I2C_PreInit(&g_softI2C);
		return 0;
	}
#if PLATFORM_BK7231T
	else if(busID == I2C_BUS_I2C1) {
		i2c_hdl = ddev_open("i2c1", &status, oflag);
	}
	else if (busID == I2C_BUS_I2C2) {
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
#else
	return 1;
#endif
}
void DRV_I2C_Close() {
	if (current_bus == I2C_BUS_SOFT) {

		return;
	}
#if PLATFORM_BK7231T
	ddev_close(i2c_hdl);
#endif
}

i2cDevice_t *g_i2c_devices = 0;

i2cBusType_t DRV_I2C_ParseBusType(const char *s) {
	if(!stricmp(s,"I2C1"))
		return I2C_BUS_I2C1;
	if(!stricmp(s,"I2C2"))
		return I2C_BUS_I2C2;
	if (!stricmp(s, "Soft"))
		return I2C_BUS_SOFT;
	return I2C_BUS_ERROR;
}
void DRV_I2C_AddNextDevice(i2cDevice_t *t) {
	t->next = g_i2c_devices;
	g_i2c_devices = t;
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
// set SoftSDA and SoftSCL pins
// startDriver I2C
// scanI2C Soft
// backlog startDriver I2C; scanI2C Soft
commandResult_t DRV_I2C_Scan(const void *context, const char *cmd, const char *args, int cmdFlags) {
	const char *i2cModuleStr;
	i2cBusType_t busType;

	Tokenizer_TokenizeString(args, 0);
	i2cModuleStr = Tokenizer_GetArg(0);

	busType = DRV_I2C_ParseBusType(i2cModuleStr);

	for (int a = 1; a < 128; a ++) {
		DRV_I2C_Begin(a, busType);
		bool bOk = Soft_I2C_Start(&g_softI2C, (a << 1) + 0);
		Soft_I2C_Stop(&g_softI2C);
		if (bOk) {
			addLogAdv(LOG_INFO, LOG_FEATURE_I2C, "Address 0x%x (dec %i)\n", a, a);
		}
		rtos_delay_milliseconds(5);
	}


	return CMD_RES_OK;
}
void DRV_I2C_Shutdown()
{
	i2cDevice_t *d = g_i2c_devices;
	g_i2c_devices = 0;
	while (d) {
		i2cDevice_t *n = d->next;
		free(d);
		d = n;
	}
}
void DRV_I2C_Init()
{

#if ENABLE_I2C_LCM1602
	DRV_I2C_LCM1602_PreInit();
#endif
#if ENABLE_I2C_ADS1115
	DRV_I2C_ADS1115_PreInit();
#endif
#if ENABLE_I2C_LCD_PCF8574
	DRV_I2C_LCD_PCF8574_PreInit();
#endif
	DRV_I2C_TC74_PreInit();
#if ENABLE_I2C_MCP23017
	DRV_I2C_MCP23017_PreInit();
#endif

	//cmddetail:{"name":"scanI2C","args":"[Soft/I2C1/I2C2]",
	//cmddetail:"descr":"Scans given I2C line for addresses. I2C driver must be started first.",
	//cmddetail:"fn":"DRV_I2C_MCP23017_MapPinToChannel","file":"i2c/drv_i2c_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("scanI2C", DRV_I2C_Scan, NULL);
}
void DRC_I2C_RunDevice(i2cDevice_t *dev)
{
	if (dev->runFrame) {
		dev->runFrame(dev);
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
void I2C_OnChannelChanged(int channel,int iVal) {
	i2cDevice_t *cur;

	cur = g_i2c_devices;
	while(cur) {
		//addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_EverySecond: going to run device of type %i with addr %i\n", cur->type, cur->addr);
		if (cur->channelChange) {
			cur->channelChange(cur, channel, iVal);
		}
		cur = cur->next;
	}
}


