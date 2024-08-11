#include <math.h>
#define CHT83XX_I2C_ADDR (0x40 << 1)
#define IS_CHT831X (sensor_id == 0x8215 || sensor_id == 0x8315)

#define CHT831X_REG_TEMP 0x00
#define CHT831X_REG_HUM 0x01
#define CHT831X_REG_STATUS 0x02
#define CHT831X_REG_CFG 0x03
#define CHT831X_REG_C_RATE 0x04
#define CHT831X_REG_TEMP_HL 0x05
#define CHT831X_REG_TEMP_LL 0x06
#define CHT831X_REG_HUM_HL 0x07
#define CHT831X_REG_HUM_LL 0x08
#define CHT831X_REG_ONESHOT 0x0F
#define CHT831X_REG_SWRST 0xFC
#define CHT831X_REG_ID 0xFE

static softI2C_t g_softI2C;

// sensor internal measurement frequency
typedef enum
{
	FREQ_120S = 0b000,
	FREQ_60S = 0b001,
	FREQ_10S = 0b010,
	FREQ_5S = 0b011,
	FREQ_1S = 0b100,
	FREQ_500MS = 0b101,
	FREQ_250MS = 0b110,
	FREQ_125MS = 0b111,
} CHT_alert_freq;

// alert source
typedef enum
{
	SRC_TEMP_OR_HUM = 0b00,
	SRC_TEMP_ONLY = 0b01,
	SRC_HUM_ONLY = 0b10,
	SRC_TEMP_AND_HUM = 0b11,
} CHT_alert_src;

// polarity, active low or active high
typedef enum
{
	POL_AL = 0b0,
	POL_AH = 0b1,
} CHT_alert_pol;

// alert fault queue
typedef enum
{
	FQ_1 = 0b00,
	FQ_2 = 0b01,
	FQ_4 = 0b10,
	FQ_6 = 0b11,
} CHT_alert_fq;

void WriteReg(uint8_t reg, int16_t data)
{
	Soft_I2C_Start(&g_softI2C, CHT83XX_I2C_ADDR);
	Soft_I2C_WriteByte(&g_softI2C, reg);
	Soft_I2C_WriteByte(&g_softI2C, (uint8_t)((data & 0xFF00) >> 8));
	Soft_I2C_WriteByte(&g_softI2C, (uint8_t)(data & 0x00FF));
	Soft_I2C_Stop(&g_softI2C);
}
