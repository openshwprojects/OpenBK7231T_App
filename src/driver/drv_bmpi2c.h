#include "../new_pins.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include <stdint.h>
#include <math.h>

#define IsBMP180 chip_id == BMP180_CHIP_ID
#define IsBMX280 chip_id == BMP280_CHIP_ID || chip_id == BME280_CHIP_ID
#define IsBME280 chip_id == BME280_CHIP_ID
#define IsBME68X chip_id == BME68X_CHIP_ID

#define I2C_MAIN_ADDR           0x77 << 1
#define I2C_ALT_ADDR            0x76 << 1

#define BMP180_CHIP_ID          0x55
#define BMP280_CHIP_ID_S1       0x56
#define BMP280_CHIP_ID_S2       0x57
#define BMP280_CHIP_ID          0x58
#define BME280_CHIP_ID          0x60
#define BME68X_CHIP_ID          0x61

#define BMX280_REG_T1           0x88
#define BMX280_REG_T2           0x8A
#define BMX280_REG_T3           0x8C

#define BMX280_REG_P1           0x8E
#define BMX280_REG_P2           0x90
#define BMX280_REG_P3           0x92
#define BMX280_REG_P4           0x94
#define BMX280_REG_P5           0x96
#define BMX280_REG_P6           0x98
#define BMX280_REG_P7           0x9A
#define BMX280_REG_P8           0x9C
#define BMX280_REG_P9           0x9E

#define BME280_REG_H1           0xA1
#define BME280_REG_H2           0xE1
#define BME280_REG_H3           0xE3
#define BME280_REG_H4           0xE4
#define BME280_REG_H5           0xE5
#define BME280_REG_H6           0xE7

#define BMP180_REG_VERSION      0xD1
#define BMP180_REG_AC1          0xAA
#define BMP180_REG_AC2          0xAC
#define BMP180_REG_AC3          0xAE
#define BMP180_REG_AC4          0xB0
#define BMP180_REG_AC5          0xB2
#define BMP180_REG_AC6          0xB4
#define BMP180_REG_B1           0xB6
#define BMP180_REG_B2           0xB8
#define BMP180_REG_MB           0xBA
#define BMP180_REG_MC           0xBC
#define BMP180_REG_MD           0xBE
#define BMP180_REG_ADC_MSB      0xF6
#define BMP180_REG_ADC_LSB      0xF7
#define BMP180_REG_ADC_XLSB     0xF8
#define BMP180_TEMP_CTRL        0x2E

#define BME280_REG_CTRL_HUM     0xF2
#define BMX280_REG_STATUS       0xF3
#define BMX280_REG_CONFIG       0xF5
#define BMX280_REG_PRESDATA     0xF7
#define BMX280_REG_TEMPDATA     0xFA
#define BMX280_REG_HUMDATA      0xFD

#define BMX_REG_CHIPID          0xD0
#define BMX_REG_RESET           0xE0
#define BMP_SOFT_RESET          0xB6
#define BMX_REG_CONTROL         0xF4

#define BME68X_REG_STATUS       0x73
#define BME68X_REG_COEFF1       0x89
#define BME68X_REG_COEFF2       0xE1
#define BME68X_REG_CONFIG       0x75
#define BME68X_REG_CTRL_MEAS    0x74
#define BME68X_REG_CTRL_HUM     0x72
#define BME68X_REG_CTRL_GAS1    0x71
#define BME68X_REG_CTRL_GAS0    0x70
#define BME68X_REG_HEATER_HEAT0 0x5A
#define BME68X_REG_HEATER_WAIT0 0x64
#define BME68X_REG_FIELD0       0x1D

// BMX280 sensor modes
typedef enum
{
	MODE_SLEEP  = 0x00, // sleep mode
	MODE_FORCED = 0x01, // forced mode
	MODE_NORMAL = 0x03, // normal mode
} BMP_mode;

// Oversampling
typedef enum
{
	SAMPLING_SKIPPED = 0x00, //skipped, output set to 0x80000
	SAMPLING_X1      = 0x01, // oversampling x1
	SAMPLING_X2      = 0x02, // x2
	SAMPLING_X4      = 0x03, // x4
	SAMPLING_X8      = 0x04, // x8
	SAMPLING_X16     = 0x05, // x16
} BMP_sampling;

// IIR filter, (FILTER - 1) where BME68x concerned
typedef enum
{
	FILTER_OFF  = 0x00,  // filter off
	FILTER_2X   = 0x01, // filter coefficient = 2
	FILTER_4X   = 0x02, // 4
	FILTER_8X   = 0x03, // 8
	FILTER_16X  = 0x04, // 16
	FILTER_32X  = 0x05, // 32,  BME68X
	FILTER_64X  = 0x06, // 64,  BME68X
	FILTER_128X = 0x07, // 128, BME68X
} BMP_filter;

// standby (inactive) time in ms (used in normal mode), t_sb[2:0]
typedef enum
{
	STANDBY_0_5  = 0x00, // standby time = 0.5 ms
	STANDBY_62_5 = 0x01, // 62.5 ms
	STANDBY_125  = 0x02, // 125 ms
	STANDBY_250  = 0x03, // 250 ms
	STANDBY_500  = 0x04, // 500 ms
	STANDBY_1000 = 0x05, // 1000 ms
	STANDBY_2000 = 0x06, // 2000 ms
	STANDBY_4000 = 0x07, // 4000 ms
} standby_time;

typedef enum
{
	BMP180_ULP      = 0x00,
	BMP180_STANDARD = 0x01,
	BMP180_HIGHRES  = 0x02,
	BMP180_UHR      = 0x03,
} BMP180_res;

struct
{
	uint16_t T1;
	int16_t  T2;
	int16_t  T3;

	uint16_t P1;
	int16_t  P2;
	int16_t  P3;
	int16_t  P4;
	int16_t  P5;
	int16_t  P6;
	int16_t  P7;
	int16_t  P8;
	int16_t  P9;

	uint8_t  H1;
	int16_t  H2;
	uint8_t  H3;
	int16_t  H4;
	int16_t  H5;
	int8_t   H6;
} BMX280_calib;

struct
{
	int16_t  AC1;
	int16_t  AC2;
	int16_t  AC3;
	uint16_t AC4;
	uint16_t AC5;
	uint16_t AC6;

	int16_t  B1;
	int16_t  B2;

	int16_t  MB;
	int16_t  MC;
	int16_t  MD;

	float temp;
} BMP180_calib;

struct
{
	uint16_t T1;
	uint16_t T2;
	uint8_t  T3;

	uint16_t P1;
	int16_t  P2;
	int8_t   P3;
	int16_t  P4;
	int16_t  P5;
	int8_t   P6;
	int8_t   P7;
	int16_t  P8;
	int16_t  P9;
	int8_t   P10;

	uint16_t H1;
	uint16_t H2;
	int8_t   H3;
	int8_t   H4;
	int8_t   H5;
	uint8_t  H6;
	int8_t   H7;

	int8_t   GH1;
	int16_t  GH2;
	int8_t   GH3;

	BMP_sampling t_samp;
	BMP_sampling p_samp;
	BMP_sampling h_samp;

	uint8_t  res_heat_range;
	int8_t   res_heat_val;
	int8_t   range_sw_err;
} BME68X_calib;

int32_t adc_T, adc_P, adc_H, t_fine;
uint8_t chip_id = 0;
BMP180_res bmp180_res = BMP180_UHR;
bool isHumidityAvail = false;

unsigned short BMP_Read(unsigned short ack)
{
	byte r;
	r = Soft_I2C_ReadByte(&g_softI2C, !ack);
	return r;
}

void BMP_Write8(uint8_t reg_addr, uint8_t _data)
{
	Soft_I2C_Start(&g_softI2C, g_softI2C.address8bit);
	Soft_I2C_WriteByte(&g_softI2C, reg_addr);
	Soft_I2C_WriteByte(&g_softI2C, _data);
	Soft_I2C_Stop(&g_softI2C);
}

uint8_t BMP_Read8(uint8_t reg_addr)
{
	Soft_I2C_Start(&g_softI2C, g_softI2C.address8bit);
	Soft_I2C_WriteByte(&g_softI2C, reg_addr);
	Soft_I2C_Stop(&g_softI2C);
	Soft_I2C_Start(&g_softI2C, g_softI2C.address8bit | 1);
	uint8_t ret = BMP_Read(0);
	Soft_I2C_Stop(&g_softI2C);

	return ret;
}

uint16_t BMP_Read16(uint8_t reg_addr)
{
	Soft_I2C_Start(&g_softI2C, g_softI2C.address8bit);
	Soft_I2C_WriteByte(&g_softI2C, reg_addr);
	Soft_I2C_Stop(&g_softI2C);
	Soft_I2C_Start(&g_softI2C, g_softI2C.address8bit | 1);
	uint8_t b1 = BMP_Read(1);
	uint8_t b2 = BMP_Read(0);
	Soft_I2C_Stop(&g_softI2C);

	return(b1 | (b2 << 8));
}

BMP_mode GetMode(int val)
{
	switch(val)
	{
	default:
	case 0: return MODE_NORMAL;
	case 1: return MODE_FORCED;
	case 2: return MODE_SLEEP;
	}
}

BMP_sampling GetSampling(int val)
{
	switch(val)
	{
	case -1: return SAMPLING_SKIPPED;
	default:
	case 1: return SAMPLING_X1;
	case 2: return SAMPLING_X2;
	case 3 ... 5: return SAMPLING_X4;
	case 6 ... 11: return SAMPLING_X8;
	case 12 ... INT_MAX: return SAMPLING_X16;
	}
}

BMP_filter GetFilter(int val)
{
	switch(val)
	{
	case 0:
	default: return FILTER_OFF;
	case 1 ... 2: return FILTER_2X;
	case 3 ... 5: return FILTER_4X;
	case 6 ... 11: return FILTER_8X;
	case 12 ... 23: return FILTER_16X;
	case 24 ... 47: return FILTER_32X;
	case 48 ... 95: return FILTER_64X;
	case 96 ... INT_MAX: return FILTER_128X;
	}
}

standby_time GetStandbyTime(int val)
{
	switch(val)
	{
	case 0 ... 31:
	default: return STANDBY_0_5;
	case 32 ... 94: return STANDBY_62_5;
	case 95 ... 174: return STANDBY_125;
	case 175 ... 374: return STANDBY_250;
	case 375 ... 750: return STANDBY_500;
	case 751 ... 1500: return STANDBY_1000;
	case 1501 ... 3000: return STANDBY_2000;
	case 3001 ... INT_MAX: return STANDBY_4000;
	}
}

void ReadCalibData_BMX280()
{
	BMX280_calib.T1 = BMP_Read16(BMX280_REG_T1);
	BMX280_calib.T2 = BMP_Read16(BMX280_REG_T2);
	BMX280_calib.T3 = BMP_Read16(BMX280_REG_T3);

	BMX280_calib.P1 = BMP_Read16(BMX280_REG_P1);
	BMX280_calib.P2 = BMP_Read16(BMX280_REG_P2);
	BMX280_calib.P3 = BMP_Read16(BMX280_REG_P3);
	BMX280_calib.P4 = BMP_Read16(BMX280_REG_P4);
	BMX280_calib.P5 = BMP_Read16(BMX280_REG_P5);
	BMX280_calib.P6 = BMP_Read16(BMX280_REG_P6);
	BMX280_calib.P7 = BMP_Read16(BMX280_REG_P7);
	BMX280_calib.P8 = BMP_Read16(BMX280_REG_P8);
	BMX280_calib.P9 = BMP_Read16(BMX280_REG_P9);

	if(IsBME280)
	{
		BMX280_calib.H1 = BMP_Read8(BME280_REG_H1);
		BMX280_calib.H2 = BMP_Read16(BME280_REG_H2);
		BMX280_calib.H3 = BMP_Read8(BME280_REG_H3);
		BMX280_calib.H4 = BMP_Read8(BME280_REG_H4) << 4 | (BMP_Read8(BME280_REG_H4 + 1) & 0x0F);
		BMX280_calib.H5 = BMP_Read8(BME280_REG_H5 + 1) << 4 | (BMP_Read8(BME280_REG_H5) >> 4);
		BMX280_calib.H6 = BMP_Read8(BME280_REG_H6);
	}
}

void ReadCalibData_BMP180()
{
	BMP180_calib.AC1 = BMP_Read16(BMP180_REG_AC1);
	BMP180_calib.AC2 = BMP_Read16(BMP180_REG_AC2);
	BMP180_calib.AC3 = BMP_Read16(BMP180_REG_AC3);
	BMP180_calib.AC4 = BMP_Read16(BMP180_REG_AC4);
	BMP180_calib.AC5 = BMP_Read16(BMP180_REG_AC5);
	BMP180_calib.AC6 = BMP_Read16(BMP180_REG_AC6);

	BMP180_calib.B1	 = BMP_Read16(BMP180_REG_B1);
	BMP180_calib.B2	 = BMP_Read16(BMP180_REG_B2);

	BMP180_calib.MB	 = BMP_Read16(BMP180_REG_MB);
	BMP180_calib.MC	 = BMP_Read16(BMP180_REG_MC);
	BMP180_calib.MD	 = BMP_Read16(BMP180_REG_MD);
}

void ReadCalibData_BME68X()
{
	Soft_I2C_Start(&g_softI2C, g_softI2C.address8bit);
	Soft_I2C_WriteByte(&g_softI2C, BME68X_REG_COEFF1);
	Soft_I2C_Stop(&g_softI2C);

	uint8_t cal1[25];
	Soft_I2C_Start(&g_softI2C, g_softI2C.address8bit | 1);
	Soft_I2C_ReadBytes(&g_softI2C, cal1, 24);
	Soft_I2C_Stop(&g_softI2C);

	Soft_I2C_Start(&g_softI2C, g_softI2C.address8bit);
	Soft_I2C_WriteByte(&g_softI2C, BME68X_REG_COEFF2);
	Soft_I2C_Stop(&g_softI2C);

	uint8_t cal2[16];
	Soft_I2C_Start(&g_softI2C, g_softI2C.address8bit | 1);
	Soft_I2C_ReadBytes(&g_softI2C, cal2, 15);
	Soft_I2C_Stop(&g_softI2C);

	BME68X_calib.T1  = cal2[9] << 8 | cal2[8];
	BME68X_calib.T2  = cal1[2] << 8 | cal1[1];
	BME68X_calib.T3  = cal1[3];

	BME68X_calib.H1  = cal2[2] << 4 | (cal2[1] & 0x0F);
	BME68X_calib.H2  = cal2[0] << 4 | cal2[1] >> 4;
	BME68X_calib.H3  = cal2[3];
	BME68X_calib.H4  = cal2[4];
	BME68X_calib.H5  = cal2[5];
	BME68X_calib.H6  = cal2[6];
	BME68X_calib.H7  = cal2[7];

	BME68X_calib.P1  = cal1[6] << 8 | cal1[5];
	BME68X_calib.P2  = cal1[8] << 8 | cal1[7];
	BME68X_calib.P3  = cal1[9];
	BME68X_calib.P4  = cal1[12] << 8 | cal1[11];
	BME68X_calib.P5  = cal1[14] << 8 | cal1[13];
	BME68X_calib.P6  = cal1[16];
	BME68X_calib.P7  = cal1[15];
	BME68X_calib.P8  = cal1[20] << 8 | cal1[19];
	BME68X_calib.P9  = cal1[22] << 8 | cal1[21];
	BME68X_calib.P10 = cal1[23];

	BME68X_calib.GH1 = cal2[14];
	BME68X_calib.GH2 = cal2[12] << 8 | cal2[13];
	BME68X_calib.GH3 = cal2[15];

	BME68X_calib.res_heat_range = (BMP_Read8(0x02) & 0x30) / 16;
	BME68X_calib.res_heat_val = (int8_t)BMP_Read8(0x00);
	BME68X_calib.range_sw_err = ((int8_t)BMP_Read8(0x04) & (int8_t)0xf0) / 16;
}

void BMXI2C_Configure(BMP_mode mode, BMP_sampling T_sampling,
	BMP_sampling P_sampling, BMP_sampling H_sampling, BMP_filter filter, standby_time standby)
{
	if(IsBMX280)
	{
		uint8_t _ctrl_meas, _config;

		_config = ((standby << 5) | (filter << 2)) & 0xFC;
		_ctrl_meas = (T_sampling << 5) | (P_sampling << 2) | mode;

		if(IsBME280)
		{
			uint8_t humid_control_val = BMP_Read8(BME280_REG_CTRL_HUM);
			humid_control_val &= ~0b00000111;
			humid_control_val |= H_sampling & 0b111;
			BMP_Write8(BME280_REG_CTRL_HUM, humid_control_val);
		}

		BMP_Write8(BMX280_REG_CONFIG, _config);
		BMP_Write8(BMX_REG_CONTROL, _ctrl_meas);
	}
	else if(IsBME68X)
	{
		BME68X_calib.t_samp = T_sampling;
		BME68X_calib.p_samp = P_sampling;
		BME68X_calib.h_samp = H_sampling;

		uint8_t _config = BMP_Read8(BME68X_REG_CONFIG);
		_config &= ~0b00011100;
		_config |= (filter & 0b111) << 2;
		BMP_Write8(BME68X_REG_CONFIG, _config);

		uint8_t _hum = BMP_Read8(BME68X_REG_CTRL_HUM);
		_hum &= ~0b00000111;
		_hum |= H_sampling & 0b111;
		BMP_Write8(BME68X_REG_CTRL_HUM, _hum);

		uint8_t _gas1 = BMP_Read8(BME68X_REG_CTRL_GAS1);
		_gas1 &= ~0b00011111;
		_gas1 |= 1 << 4;
		_gas1 |= 0;
		BMP_Write8(BME68X_REG_CTRL_GAS1, _gas1);

		uint8_t _gas0 = BMP_Read8(BME68X_REG_CTRL_GAS0);
		_gas0 &= ~0b00001000;
		_gas0 |= true << 3;
		BMP_Write8(BME68X_REG_CTRL_GAS0, _gas0);
	}
}

void BMP_SoftReset()
{
	BMP_Write8(BMX_REG_RESET, BMP_SOFT_RESET);
}

bool BMP_BasicInit()
{
	chip_id = BMP_Read8(BMX_REG_CHIPID);

	switch(chip_id)
	{
	case BMP180_CHIP_ID:
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "BMP085/BMP180 detected!");
		g_chipName = "BMP180";
		uint8_t ver = BMP_Read8(BMP180_REG_VERSION);
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "Sensor version: %#02x", ver);
		ReadCalibData_BMP180();
		return true;
	// 0x56 and 0x57 are samples, 0x58 are mass production
	case BMP280_CHIP_ID_S1:
	case BMP280_CHIP_ID_S2:
	case BMP280_CHIP_ID:
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "BMP280 detected!");
		g_chipName = "BMP280";
		break;
	case BME280_CHIP_ID:
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "BME280 detected!");
		g_chipName = "BME280";
		isHumidityAvail = true;
		break;
	case BME68X_CHIP_ID:
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "BME68X detected!");
		g_chipName = "BME68X";
		isHumidityAvail = true;
		break;
	case 0xFF:
		ADDLOG_WARN(LOG_FEATURE_SENSOR, "No sensor detected on selected address!");
		uint8_t altAddr;

		if(g_softI2C.address8bit == I2C_MAIN_ADDR) altAddr = I2C_ALT_ADDR;
		else altAddr = I2C_MAIN_ADDR;

		Soft_I2C_Start(&g_softI2C, altAddr);
		Soft_I2C_WriteByte(&g_softI2C, BMX_REG_CHIPID);
		Soft_I2C_Stop(&g_softI2C);
		Soft_I2C_Start(&g_softI2C, altAddr | 1);
		uint8_t eaddr = BMP_Read(0);
		Soft_I2C_Stop(&g_softI2C);

		if(eaddr != 0xFF)
		{
			ADDLOG_WARN(LOG_FEATURE_SENSOR, "Detected sensor on alt address, id: %#02x, edit conf!", eaddr);
		}
		else
		{
			ADDLOG_WARN(LOG_FEATURE_SENSOR, "No sensor detected on alternative address, check wiring!");
		}

		return false;
	default:
		ADDLOG_ERROR(LOG_FEATURE_SENSOR, "BMxx80 wrong ID! Detected: %#02x", chip_id);
		return false;
	}

	BMP_Write8(BMX_REG_RESET, BMP_SOFT_RESET);
	delay_ms(5);

	if(IsBMX280)
	{
		delay_ms(100);
		while((BMP_Read8(BMX280_REG_STATUS) & 0x01) == 0x01)
			delay_ms(100);
		ReadCalibData_BMX280();
	}
	else if(IsBME68X)
	{
		//while((BMP_Read8(BME68X_REG_STATUS) & 0x01) == 0x01)
		//	delay_ms(5);
		ReadCalibData_BME68X();
		delay_ms(5);
	}

	return true;
}

void BMX280_Update()
{
	Soft_I2C_Start(&g_softI2C, g_softI2C.address8bit);
	Soft_I2C_WriteByte(&g_softI2C, BMX280_REG_PRESDATA);
	Soft_I2C_Stop(&g_softI2C);
	Soft_I2C_Start(&g_softI2C, g_softI2C.address8bit | 1);

	int32_t adc = ((BMP_Read(1) & 0xFF) << 16) | ((BMP_Read(1) & 0xFF) << 8) | (BMP_Read(0) & 0xFF);
	adc_P = adc >> 4;

	Soft_I2C_Stop(&g_softI2C);

	Soft_I2C_Start(&g_softI2C, g_softI2C.address8bit);
	Soft_I2C_WriteByte(&g_softI2C, BMX280_REG_TEMPDATA);
	Soft_I2C_Stop(&g_softI2C);
	Soft_I2C_Start(&g_softI2C, g_softI2C.address8bit | 1);
	adc = ((BMP_Read(1) & 0xFF) << 16) | ((BMP_Read(1) & 0xFF) << 8) | (BMP_Read(0) & 0xFF);

	adc_T = adc >> 4;

	if(IsBME280)
	{
		uint8_t data1 = BMP_Read(1);
		uint8_t data2 = BMP_Read(0);
		adc_H = ((data1 & 0xFF) << 8) | (data2 & 0xFF);
	}
	Soft_I2C_Stop(&g_softI2C);
}

int32_t BMX280_ReadTemperature()
{
	int32_t var1, var2, t1, t2, t3;

	t1 = BMX280_calib.T1;
	t2 = BMX280_calib.T2;
	t3 = BMX280_calib.T3;

	var1 = (((adc_T >> 3) - (t1 << 1)) * t2) >> 11;
	var2 = (((((adc_T >> 4) - t1) * ((adc_T >> 4) - t1)) >> 12) * t3) >> 14;

	t_fine = var1 + var2;

	return (t_fine * 5 + 128) / 256.0f;
}

uint32_t BMX280_ReadPressure()
{
	int64_t var1, var2, p;

	int64_t p1 = BMX280_calib.P1;
	int64_t p2 = BMX280_calib.P2;
	int64_t p3 = BMX280_calib.P3;
	int64_t p4 = BMX280_calib.P4;
	int64_t p5 = BMX280_calib.P5;
	int64_t p6 = BMX280_calib.P6;
	int64_t p7 = BMX280_calib.P7;
	int64_t p8 = BMX280_calib.P8;
	int64_t p9 = BMX280_calib.P9;

	var1 = (int64_t)(t_fine) - 128000;
	var2 = var1 * var1 * p6;
	var2 = var2 + ((var1 * p5) << 17);
	var2 = var2 + (p4 << 35);
	var1 = ((var1 * var1 * p3) >> 8) + ((var1 * p2) << 12);
	var1 = (((int64_t)(1) << 47) + var1) * p1 >> 33;

	if(var1 == 0) return 0;

	p = 1048576 - (int64_t)adc_P;
	p = (((p << 31) - var2) * 3125) / var1;
	var1 = (p9 * (p >> 13) * (p >> 13)) >> 25;
	var2 = (p8 * p) >> 19;

	p = ((p + var1 + var2) >> 8) + (p7 << 4);
	return p / 256.0f;
}

uint32_t BME280_ReadHumidity()
{
	if(adc_H == 0x8000) return 0;

	int32_t h1 = BMX280_calib.H1;
	int32_t h2 = BMX280_calib.H2;
	int32_t h3 = BMX280_calib.H3;
	int32_t h4 = BMX280_calib.H4;
	int32_t h5 = BMX280_calib.H5;
	int32_t h6 = BMX280_calib.H6;

	int32_t v_x1_u32r = t_fine - 76800;

	v_x1_u32r = ((((adc_H << 14) - (h4 << 20) - (h5 * v_x1_u32r)) + 16384) >> 15) *
		(((((((v_x1_u32r * h6) >> 10) * (((v_x1_u32r * h3) >> 11) + 32768)) >> 10) + 2097152) * h2 + 8192) >> 14);

	v_x1_u32r = v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * h1) >> 4);

	v_x1_u32r = v_x1_u32r < 0 ? 0 : v_x1_u32r;
	v_x1_u32r = v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r;
	float h = v_x1_u32r >> 12;

	return (h / 1024.0f) * 10;
}

uint32_t BMP180_ReadData(int32_t* temp)
{
	int32_t  UT = 0;
	int32_t  UP = 0;
	int32_t  B3 = 0;
	int32_t  B5 = 0;
	int32_t  B6 = 0;
	int32_t  X1 = 0;
	int32_t  X2 = 0;
	int32_t  X3 = 0;
	int32_t  pressure = 0;
	uint32_t B4 = 0;
	uint32_t B7 = 0;

	BMP_Write8(BMX_REG_CONTROL, BMP180_TEMP_CTRL);
	delay_ms(5);
	UT = BMP_Read16(BMP180_REG_ADC_MSB);
	uint8_t delay = 0, res = 0;
	switch(bmp180_res)
	{
	case BMP180_ULP: res = 0x34; delay = 5; break;
	case BMP180_STANDARD: res = 0x74; delay = 8; break;
	case BMP180_HIGHRES: res = 0xB4; delay = 14; break;
	case BMP180_UHR: res = 0xF4; delay = 26; break;
	}
	BMP_Write8(BMX_REG_CONTROL, res);
	delay_ms(delay);
	Soft_I2C_Start(&g_softI2C, g_softI2C.address8bit);
	Soft_I2C_WriteByte(&g_softI2C, BMP180_REG_ADC_MSB);
	Soft_I2C_Stop(&g_softI2C);
	Soft_I2C_Start(&g_softI2C, g_softI2C.address8bit | 1);
	UP = BMP_Read(1);
	UP <<= 8 | BMP_Read(1);
	UP <<= 8 | BMP_Read(0);
	Soft_I2C_Stop(&g_softI2C);
	UP >>= (8 - bmp180_res);

	int32_t BX1 = ((UT - (int32_t)BMP180_calib.AC6) * (int32_t)BMP180_calib.AC5) >> 15;
	int32_t BX2 = ((int32_t)BMP180_calib.MC << 11) / (X1 + (int32_t)BMP180_calib.MD);
	B5 = BX1 + BX2;
	*temp = ((B5 + 8) >> 4);

	B6 = B5 - 4000;
	X1 = ((int32_t)BMP180_calib.B2 * ((B6 * B6) >> 12)) >> 11;
	X2 = ((int32_t)BMP180_calib.AC2 * B6) >> 11;
	X3 = X1 + X2;
	B3 = ((((int32_t)BMP180_calib.AC1 * 4 + X3) << bmp180_res) + 2) >> 2;

	X1 = ((int32_t)BMP180_calib.AC3 * B6) >> 13;
	X2 = ((int32_t)BMP180_calib.B1 * ((B6 * B6) >> 12)) >> 16;
	X3 = ((X1 + X2) + 2) >> 2;
	B4 = ((uint32_t)BMP180_calib.AC4 * (uint32_t)(X3 + 32768)) >> 15;
	B7 = ((uint32_t)UP - B3) * (uint32_t)(50000 >> bmp180_res);

	if(B4 == 0) return 0;

	if(B7 < 0x80000000) pressure = (B7 * 2) / B4;
	else pressure = (B7 / B4) * 2;
	X1 = (pressure >> 8) * (pressure >> 8);
	X1 = (X1 * 3038) >> 16;
	X2 = (-7357 * pressure) >> 16;
	return pressure + ((X1 + X2 + (int32_t)3791) >> 4);
}

void BME68X_Update()
{
	uint8_t meas_control = 0;
	meas_control |= (BME68X_calib.t_samp & 0b111) << 5;
	meas_control |= (BME68X_calib.p_samp & 0b111) << 2;
	meas_control |= 0b01;
	BMP_Write8(BME68X_REG_CTRL_MEAS, meas_control);

	uint32_t tph_dur;
	uint32_t meas_cycles;
	const uint8_t os_to_meas_cycles[6] = { 0, 1, 2, 4, 8, 16 };

	meas_cycles = os_to_meas_cycles[BME68X_calib.t_samp];
	meas_cycles += os_to_meas_cycles[BME68X_calib.p_samp];
	meas_cycles += os_to_meas_cycles[BME68X_calib.h_samp];
	tph_dur = meas_cycles * 1963u;
	tph_dur += 477 * 4;
	tph_dur += 477 * 5;
	tph_dur += 500;
	tph_dur /= 1000;
	tph_dur += 1;
	delay_ms(tph_dur);

	Soft_I2C_Start(&g_softI2C, g_softI2C.address8bit);
	Soft_I2C_WriteByte(&g_softI2C, BME68X_REG_FIELD0);
	Soft_I2C_Stop(&g_softI2C);

	uint8_t data[15];
	Soft_I2C_Start(&g_softI2C, g_softI2C.address8bit | 1);
	Soft_I2C_ReadBytes(&g_softI2C, data, 14);
	Soft_I2C_Stop(&g_softI2C);

	adc_T = ((uint32_t)(data[5]) << 12) | ((uint32_t)(data[6]) << 4) | ((uint32_t)(data[7]) >> 4);
	adc_P = ((uint32_t)(data[2]) << 12) | ((uint32_t)(data[3]) << 4) | ((uint32_t)(data[4]) >> 4);
	adc_H = ((uint32_t)(data[8]) << 8) | (uint32_t)(data[9]);
	//uint16_t raw_gas = (uint16_t)((uint32_t)data[13] * 4 | (((uint32_t)data[14]) / 64));
	//uint8_t gas_range = data[14] & 0x0F;
}

int32_t BME68X_ReadTemperature()
{
	int64_t var1;
	int64_t var2;
	int16_t temperature;

	var1 = ((((adc_T >> 3) - ((int32_t)BME68X_calib.T1 << 1))) *
		((int32_t)BME68X_calib.T2)) >> 11;
	var2 = (((((adc_T >> 4) - ((int32_t)BME68X_calib.T1)) *
		((adc_T >> 4) - ((int32_t)BME68X_calib.T1))) >> 12) *
		((int32_t)BME68X_calib.T3)) >> 14;
	t_fine = (int32_t)(var1 + var2);
	return temperature = (t_fine * 5 + 128) >> 8;
}

uint32_t BME68X_ReadPressure()
{
	int32_t var1;
	int32_t var2;
	int32_t var3;
	int32_t var4;
	int32_t pressure;

	var1 = (t_fine >> 1) - 64000;
	var2 = ((((var1 >> 2) * (var1 >> 2)) >> 11) * (int32_t)BME68X_calib.P6) >> 2;
	var2 = ((var2) * (int32_t)BME68X_calib.P6) >> 2;
	var2 = var2 + ((var1 * (int32_t)BME68X_calib.P5) << 1);
	var2 = (var2 >> 2) + ((int32_t)BME68X_calib.P4 << 16);
	var1 = (((var1 >> 2) * (var1 >> 2)) >> 13);
	var1 = (((var1) * ((int32_t)BME68X_calib.P3 << 5)) >> 3) + (((int32_t)BME68X_calib.P2 * var1) >> 1);
	var1 = var1 >> 18;
	var1 = ((32768 + var1) * (int32_t)BME68X_calib.P1) >> 15;
	pressure = 1048576 - adc_P;
	pressure = (int32_t)((pressure - (var2 >> 12)) * ((uint32_t)3125));
	var4 = (1 << 31);
	pressure = (pressure >= var4) ? ((pressure / (uint32_t)var1) << 1)
		: ((pressure << 1) / (uint32_t)var1);
	var1 = ((int32_t)BME68X_calib.P9 * (int32_t)(((pressure >> 3) * (pressure >> 3)) >> 13)) >> 12;
	var2 = ((int32_t)(pressure >> 2) * (int32_t)BME68X_calib.P8) >> 13;
	var3 = ((int32_t)(pressure >> 8) * (int32_t)(pressure >> 8)
		* (int32_t)(pressure >> 8)
		* (int32_t)BME68X_calib.P10) >> 17;
	pressure = (int32_t)(pressure)+((var1 + var2 + var3 + ((int32_t)BME68X_calib.P7 << 7)) >> 4);

	return (uint32_t)pressure;
}

uint32_t BME68X_ReadHumidity()
{
	int32_t var1;
	int32_t var2;
	int32_t var3;
	int32_t var4;
	int32_t var5;
	int32_t var6;
	int32_t temp_scaled;
	int32_t humidity;

	temp_scaled = ((t_fine * 5) + 128) >> 8;
	var1 = (int32_t)(adc_H - ((int32_t)((int32_t)BME68X_calib.H1 << 4))) -
		(((temp_scaled * (int32_t)BME68X_calib.H3) / ((int32_t)100)) >> 1);
	var2 = ((int32_t)BME68X_calib.H2 *
		(((temp_scaled * (int32_t)BME68X_calib.H4) / ((int32_t)100)) +
			(((temp_scaled * ((temp_scaled * (int32_t)BME68X_calib.H5) / ((int32_t)100))) >> 6) /
				((int32_t)100)) + (int32_t)(1 << 14))) >> 10;
	var3 = var1 * var2;
	var4 = (int32_t)BME68X_calib.H6 << 7;
	var4 = ((var4)+((temp_scaled * (int32_t)BME68X_calib.H7) / ((int32_t)100))) >> 4;
	var5 = ((var3 >> 14) * (var3 >> 14)) >> 10;
	var6 = (var4 * var5) >> 1;
	humidity = (((var3 + var6) >> 10) * ((int32_t)1000)) >> 12;

	if(humidity > 100000)
		humidity = 100000;
	else if(humidity < 0)
		humidity = 0;

	return (uint32_t)(humidity * 0.01f);
}
