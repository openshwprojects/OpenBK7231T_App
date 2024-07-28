#define CHT83XX_I2C_ADDR (0x40 << 1)
#define IS_CHT831X (sensor_id == 0x8215 || sensor_id == 0x8315)

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