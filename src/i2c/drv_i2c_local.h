
enum i2cDeviceType_e {
	I2CDEV_UNKNOWN,
	I2CDEV_TC74,
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

void DRV_I2C_Write(UINT8 addr, UINT8 data);
void DRV_I2C_Read(UINT8 addr, UINT8 *data);
int DRV_I2C_Begin(int dev_adr, int busID);
void DRV_I2C_Close();

