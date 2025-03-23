#ifndef __DRV_I2C_LOCAL_H__
#define __DRV_I2C_LOCAL_H__

#include "../cmnds/cmd_public.h"

enum i2cDeviceType_e {
	I2CDEV_UNKNOWN,
	I2CDEV_TC74,
	I2CDEV_MCP23017,
	I2CDEV_LCD_PCF8574,
	I2CDEV_ADS1115,
}; 

typedef enum i2cBusType_e {
	I2C_BUS_ERROR,
	I2C_BUS_I2C1,
	I2C_BUS_I2C2,
	I2C_BUS_SOFT,
} i2cBusType_t;

typedef struct i2cDevice_s {
	int busType;
	int addr;
	int type;
	void(*runFrame)(struct i2cDevice_s *ptr);
	void(*channelChange)(struct i2cDevice_s *dev, int channel, int iVal);
	struct i2cDevice_s *next;
} i2cDevice_t;


void DRV_I2C_Write(byte addr, byte data);
void DRV_I2C_WriteBytes(byte addr, byte *data, int len);
void DRV_I2C_WriteBytesSingle(byte *data, int len);
void DRV_I2C_ReadBytes(byte addr, byte *data, int size);
void DRV_I2C_Read(byte addr, byte *data);
int DRV_I2C_Begin(int dev_adr, int busID);
void DRV_I2C_Close();

i2cBusType_t DRV_I2C_ParseBusType(const char *s);
i2cDevice_t *DRV_I2C_FindDevice(int busType,int address);
i2cDevice_t *DRV_I2C_FindDeviceExt(int busType,int address, int devType);
void DRV_I2C_AddNextDevice(i2cDevice_t *t);


// drv_i2c_mcp23017.c
void DRV_I2C_MCP23017_PreInit();

// drv_i2c_tc74.c
void DRV_I2C_TC74_PreInit();

// drv_i2c_lcd_pcf8574t.c
void DRV_I2C_LCD_PCF8574_PreInit();

void DRV_I2C_ADS1115_PreInit();

void DRV_I2C_LCM1602_PreInit();

#endif // __DRV_I2C_LOCAL_H__