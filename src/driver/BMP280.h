///////////////////////////////////////////////////////////////////////////
////                                                                   ////
////                               BMP280.c                            ////
////                                                                   ////
////               Driver for mikroC PRO for PIC compiler              ////
////                                                                   ////
//// Driver for Bosch BMP280 sensor. This sensor can read temperature  ////
//// and pressure.                                                     ////
//// This driver only supports I2C mode, it doesn't support SPI mode.  ////
////                                                                   ////
///////////////////////////////////////////////////////////////////////////
////                                                                   ////
////                     https://simple-circuit.com/                   ////
////                                                                   ////
///////////////////////////////////////////////////////////////////////////

#include <stdint.h>

#define BMP280_CHIP_ID        0x58
#define BME280_CHIP_ID        0x60

#define BMP280_REG_DIG_T1     0x88
#define BMP280_REG_DIG_T2     0x8A
#define BMP280_REG_DIG_T3     0x8C

#define BMP280_REG_DIG_P1     0x8E
#define BMP280_REG_DIG_P2     0x90
#define BMP280_REG_DIG_P3     0x92
#define BMP280_REG_DIG_P4     0x94
#define BMP280_REG_DIG_P5     0x96
#define BMP280_REG_DIG_P6     0x98
#define BMP280_REG_DIG_P7     0x9A
#define BMP280_REG_DIG_P8     0x9C
#define BMP280_REG_DIG_P9     0x9E

#define BMP280_REG_CHIPID     0xD0
#define BMP280_REG_SOFTRESET  0xE0

#define BMP280_REG_STATUS     0xF3
#define BMP280_REG_CONTROL    0xF4
#define BMP280_REG_CONFIG     0xF5
#define BMP280_REG_PRESS_MSB  0xF7

int32_t adc_T, adc_P, t_fine;

// BMP280 sensor modes, register ctrl_meas mode[1:0]
typedef enum
{
  MODE_SLEEP  = 0x00,  // sleep mode
  MODE_FORCED = 0x01,  // forced mode
  MODE_NORMAL = 0x03   // normal mode
} BMP280_mode;

// oversampling setting. osrs_t[2:0], osrs_p[2:0]
typedef enum
{
  SAMPLING_SKIPPED = 0x00,  //skipped, output set to 0x80000
  SAMPLING_X1      = 0x01,  // oversampling x1
  SAMPLING_X2      = 0x02,  // oversampling x2
  SAMPLING_X4      = 0x03,  // oversampling x4
  SAMPLING_X8      = 0x04,  // oversampling x8
  SAMPLING_X16     = 0x05   // oversampling x16
} BMP280_sampling;

// filter setting filter[2:0]
typedef enum
{
  FILTER_OFF = 0x00,  // filter off
  FILTER_2   = 0x01,  // filter coefficient = 2
  FILTER_4   = 0x02,  // filter coefficient = 4
  FILTER_8   = 0x03,  // filter coefficient = 8
  FILTER_16  = 0x04   // filter coefficient = 16
} BMP280_filter;

// standby (inactive) time in ms (used in normal mode), t_sb[2:0]
typedef enum
{
  STANDBY_0_5   =  0x00,  // standby time = 0.5 ms
  STANDBY_62_5  =  0x01,  // standby time = 62.5 ms
  STANDBY_125   =  0x02,  // standby time = 125 ms
  STANDBY_250   =  0x03,  // standby time = 250 ms
  STANDBY_500   =  0x04,  // standby time = 500 ms
  STANDBY_1000  =  0x05,  // standby time = 1000 ms
  STANDBY_2000  =  0x06,  // standby time = 2000 ms
  STANDBY_4000  =  0x07   // standby time = 4000 ms
} standby_time;

struct
{
  uint16_t dig_T1;
  int16_t  dig_T2;
  int16_t  dig_T3;

  uint16_t dig_P1;
  int16_t  dig_P2;
  int16_t  dig_P3;
  int16_t  dig_P4;
  int16_t  dig_P5;
  int16_t  dig_P6;
  int16_t  dig_P7;
  int16_t  dig_P8;
  int16_t  dig_P9;
} BMP280_calib;

// writes 1 byte '_data' to register 'reg_addr'
void BMP280_Write8(uint8_t reg_addr, uint8_t _data)
{
  BMP280_Start();
  BMP280_Write(g_softI2C.address8bit);
  BMP280_Write(reg_addr);
  BMP280_Write(_data);
  BMP280_Stop();
}

// reads 8 bits from register 'reg_addr'
uint8_t BMP280_Read8(uint8_t reg_addr)
{
  uint8_t ret;

  BMP280_Start();
  BMP280_Write(g_softI2C.address8bit);
  BMP280_Write(reg_addr);
  Soft_I2C_Stop(&g_softI2C);
  BMP280_Start();
  BMP280_Write(g_softI2C.address8bit | 1);
  ret = BMP280_Read(0);
  BMP280_Stop();

  return ret;
}

// reads 16 bits from register 'reg_addr'
uint16_t BMP280_Read16(uint8_t reg_addr)
{
  union
  {
    uint8_t  b[2];
    uint16_t w;
  } ret;

  BMP280_Start();
  BMP280_Write(g_softI2C.address8bit);
  BMP280_Write(reg_addr);
  Soft_I2C_Stop(&g_softI2C);
  BMP280_Start();
  BMP280_Write(g_softI2C.address8bit | 1);
  ret.b[0] = BMP280_Read(1);
  ret.b[1] = BMP280_Read(0);
  BMP280_Stop();

  return(ret.w);
}

// BMP280 sensor configuration function
void BMP280_Configure(BMP280_mode mode, BMP280_sampling T_sampling,
                      BMP280_sampling P_sampling, BMP280_filter filter, standby_time standby)
{
  uint8_t  _ctrl_meas, _config;

  _config = ((standby << 5) | (filter << 2)) & 0xFC;
  _ctrl_meas = (T_sampling << 5) | (P_sampling << 2) | mode;

  BMP280_Write8(BMP280_REG_CONFIG,  _config);
  BMP280_Write8(BMP280_REG_CONTROL, _ctrl_meas);
}

// initializes the BMP280 sensor, returns 1 if OK and 0 if error
uint8_t BMP280_begin(BMP280_mode mode,
                  BMP280_sampling T_sampling,
                  BMP280_sampling P_sampling,
                  BMP280_filter filter,
                  standby_time  standby)
{
	int id = BMP280_Read8(BMP280_REG_CHIPID);
	if (id == BMP280_CHIP_ID) {
		addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "BMP280 detected!");
	} else if (id == BME280_CHIP_ID) {
		addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "BME280 detected!");
	}
	else {
		addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "BMx280 wrong ID!");
		return 0;
	}

  // reset the BMP280 with soft reset
  BMP280_Write8(BMP280_REG_SOFTRESET, 0xB6);
  delay_ms(100);

  // if NVM data are being copied to image registers, wait 100 ms
  while ( (BMP280_Read8(BMP280_REG_STATUS) & 0x01) == 0x01 )
    delay_ms(100);

  BMP280_calib.dig_T1 = BMP280_Read16(BMP280_REG_DIG_T1);
  BMP280_calib.dig_T2 = BMP280_Read16(BMP280_REG_DIG_T2);
  BMP280_calib.dig_T3 = BMP280_Read16(BMP280_REG_DIG_T3);

  BMP280_calib.dig_P1 = BMP280_Read16(BMP280_REG_DIG_P1);
  BMP280_calib.dig_P2 = BMP280_Read16(BMP280_REG_DIG_P2);
  BMP280_calib.dig_P3 = BMP280_Read16(BMP280_REG_DIG_P3);
  BMP280_calib.dig_P4 = BMP280_Read16(BMP280_REG_DIG_P4);
  BMP280_calib.dig_P5 = BMP280_Read16(BMP280_REG_DIG_P5);
  BMP280_calib.dig_P6 = BMP280_Read16(BMP280_REG_DIG_P6);
  BMP280_calib.dig_P7 = BMP280_Read16(BMP280_REG_DIG_P7);
  BMP280_calib.dig_P8 = BMP280_Read16(BMP280_REG_DIG_P8);
  BMP280_calib.dig_P9 = BMP280_Read16(BMP280_REG_DIG_P9);

  BMP280_Configure(mode, T_sampling, P_sampling, filter, standby);

  return 1;
}

// Takes a new measurement, for forced mode only!
// Returns 1 if ok and 0 if error (sensor is not in sleep mode)
uint8_t BMP280_ForcedMeasurement()
{
  uint8_t ctrl_meas_reg = BMP280_Read8(BMP280_REG_CONTROL);

  if ( (ctrl_meas_reg & 0x03) != 0x00 )
    return 0;   // sensor is not in sleep mode

  // set sensor to forced mode
  BMP280_Write8(BMP280_REG_CONTROL, ctrl_meas_reg | 1);
  // wait for conversion complete
  while (BMP280_Read8(BMP280_REG_STATUS) & 0x08)
    delay_ms(1);

  return 1;
}

// read (updates) adc_P, adc_T and adc_H from BMP280 sensor
void BMP280_Update()
{
  union
  {
    uint8_t  b[4];
    uint32_t dw;
  } ret;
  ret.b[3] = 0x00;

  BMP280_Start();
  BMP280_Write(g_softI2C.address8bit);
  BMP280_Write(BMP280_REG_PRESS_MSB);
  Soft_I2C_Stop(&g_softI2C);
  BMP280_Start();
  BMP280_Write(g_softI2C.address8bit | 1);
  ret.b[2] = BMP280_Read(1);
  ret.b[1] = BMP280_Read(1);
  ret.b[0] = BMP280_Read(1);

  adc_P = (ret.dw >> 4) & 0xFFFFF;  

  ret.b[2] = BMP280_Read(1);
  ret.b[1] = BMP280_Read(1);
  ret.b[0] = BMP280_Read(0);
  BMP280_Stop();

  adc_T = (ret.dw >> 4) & 0xFFFFF;
}

// Reads temperature from BMP280 sensor.
// Temperature is stored in hundredths C (output value of "5123" equals 51.23 DegC).
// Temperature value is saved to *temp, returns 1 if OK and 0 if error.
uint8_t BMP280_readTemperature(int32_t *temp)
{
  int32_t var1, var2;

  BMP280_Update();

  // calculate temperature
  var1 = ((((adc_T / 8) - ((int32_t)BMP280_calib.dig_T1 * 2))) *
         ((int32_t)BMP280_calib.dig_T2)) / 2048;

  var2 = (((((adc_T / 16) - ((int32_t)BMP280_calib.dig_T1)) *
         ((adc_T / 16) - ((int32_t)BMP280_calib.dig_T1))) / 4096) *
         ((int32_t)BMP280_calib.dig_T3)) / 16384;

  t_fine = var1 + var2;

  *temp = (t_fine * 5 + 128) / 256;

  return 1;
}

// Reads pressure from BMP280 sensor.
// Pressure is stored in Pa (output value of "96386" equals 96386 Pa = 963.86 hPa).
// Pressure value is saved to *pres, returns 1 if OK and 0 if error.
uint8_t BMP280_readPressure(uint32_t *pres)
{
  int32_t var1, var2;
  uint32_t p;

  // calculate pressure
  var1 = (((int32_t)t_fine) / 2) - (int32_t)64000;
  var2 = (((var1/4) * (var1/4)) / 2048 ) * ((int32_t)BMP280_calib.dig_P6);

  var2 = var2 + ((var1 * ((int32_t)BMP280_calib.dig_P5)) * 2);
  var2 = (var2/4) + (((int32_t)BMP280_calib.dig_P4) * 65536);

  var1 = ((((int32_t)BMP280_calib.dig_P3 * (((var1/4) * (var1/4)) / 8192 )) / 8) +
         ((((int32_t)BMP280_calib.dig_P2) * var1)/2)) / 262144;

  var1 =((((32768 + var1)) * ((int32_t)BMP280_calib.dig_P1)) / 32768);

  if (var1 == 0)
    return 0; // avoid exception caused by division by zero

  p = (((uint32_t)(((int32_t)1048576) - adc_P) - (var2 / 4096))) * 3125;

  if (p < 0x80000000)
    p = (p * 2) / ((uint32_t)var1);

  else
    p = (p / (uint32_t)var1) * 2;

  var1 = (((int32_t)BMP280_calib.dig_P9) * ((int32_t)(((p/8) * (p/8)) / 8192))) / 4096;
  var2 = (((int32_t)(p/4)) * ((int32_t)BMP280_calib.dig_P8)) / 8192;

  p = (uint32_t)((int32_t)p + ((var1 + var2 + (int32_t)BMP280_calib.dig_P7) / 16));

  *pres = p;

  return 1;
}

// end of driver code.