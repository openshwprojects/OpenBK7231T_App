#ifndef __DRV_I2C_LOCAL_H__
#define __DRV_I2C_LOCAL_H__

enum i2cDeviceType_e {
	I2CDEV_UNKNOWN,
	I2CDEV_TC74,
	I2CDEV_MCP23017,
};

typedef enum i2cBusType_e {
	I2C_BUS_ERROR,
	I2C_BUS_I2C1,
	I2C_BUS_I2C2,

} i2cBusType_t;

typedef struct i2cDevice_s {
	int busType;
	int addr;
	int type;
	struct i2cDevice_s *next;
} i2cDevice_t;

typedef struct i2cDevice_TC74_s {
	i2cDevice_t base;
	// private TC74 variables
	// Our channel index to save the result temp
	int targetChannel;
} i2cDevice_TC74_t;

// https://www.elektroda.pl/rtvforum/viewtopic.php?t=3880540&highlight=
typedef struct i2cDevice_SM2135_s {
	i2cDevice_t base;
	// private SM2135 variables
	// Input channel indices.
	// Device will listen to changes in those channels and update accordingly.
	int sourceChannel_R;
	int sourceChannel_G;
	int sourceChannel_B;
	int sourceChannel_C;
	int sourceChannel_W;
} i2cDevice_SM2135_t;

// Right now, MCP23017 port expander supports only output mode
// (map channel to MCP23017 output)
typedef struct i2cDevice_MCP23017_s {
	i2cDevice_t base;
	// private MCP23017 variables
	// Channel indices (0xff = none)
	byte pinMapping[16];
	// is pin an output or input?
	//int pinDirections;
} i2cDevice_MCP23017_t;

void DRV_I2C_Write(byte addr, byte data);
void DRV_I2C_Read(byte addr, byte *data);
int DRV_I2C_Begin(int dev_adr, int busID);
void DRV_I2C_Close();

i2cBusType_t DRV_I2C_ParseBusType(const char *s);
i2cDevice_t *DRV_I2C_FindDevice(int busType,int address);
i2cDevice_t *DRV_I2C_FindDeviceExt(int busType,int address, int devType);


// drv_i2c_mcp23017.c
void DRV_I2C_MCP23017_RunDevice(i2cDevice_t *dev);
int DRV_I2C_MCP23017_MapPinToChannel(const void *context, const char *cmd, const char *args);
void DRV_I2C_MCP23017_OnChannelChanged(i2cDevice_t *dev, int channel, int iVal);

// drv_i2c_tc74.c
void DRV_I2C_TC74_RunDevice(i2cDevice_t *dev);

#endif // __DRV_I2C_LOCAL_H__