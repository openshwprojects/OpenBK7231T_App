#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"
#include "../hal/hal_pins.h"
#include "../hal/hal_os_wrapper.h"

#include "drv_shtc3.h"

static byte channel_temp = 0, channel_humid = 0;
// TODO static byte g_sht_secondsUntilNextMeasurement = 1, g_sht_secondsBetweenMeasurements = 10;
static float g_temp = 0.0, g_humid = 0.0;
// TODO ? static float g_caltemp = 0.0, g_calhum = 0.0;
static softI2C_t g_softI2C;
static int8_t g_pin_power = -1;
static int    g_error_channel = -1;


int shtc3_get_id(uint16_t *id);
int shtc3_get_temp_and_hum(float *temp, float *hum);
int shtc3_get_temp_and_hum_lpm(float *temp, float *hum);
int shtc3_sleep();
int shtc3_wakeup(); 
int shtc3_soft_reset();
static int8_t shtc3_reg_read(uint8_t *data, uint32_t data_len);
static int8_t shtc3_reg_write(uint16_t data);
static bool check_crc(const uint8_t *data, uint16_t count, uint8_t checksum);
static float calc_temp(uint16_t raw_temp);
static float calc_hum(uint16_t raw_hum);

void SHTC3_Measure()
{
	if (g_pin_power != -1) {
		HAL_PIN_SetOutputValue(g_pin_power&0x7F,  (g_pin_power&0x80) >> 7); // power on
	}
	rtos_delay_milliseconds(100);
	shtc3_wakeup();
	rtos_delay_milliseconds(100);
	if (shtc3_get_temp_and_hum_lpm(&g_temp, &g_humid)) {
		//addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SHTC3: Read measure ERROR");
		if (g_error_channel != -1) {
			CHANNEL_Set(g_error_channel, 1, 0);
		}
	} else {
		//addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SHTC3: Measure OK");
		if (g_error_channel != -1) {
			CHANNEL_Set(g_error_channel, 0, 0);
		}
	};
	if (g_pin_power != -1) {
		HAL_PIN_SetOutputValue(g_pin_power&0x7F, ~(g_pin_power&0x80) >> 7); // power off
	}	
	CHANNEL_Set(channel_temp, (int)(g_temp * 10), 0);
	CHANNEL_Set(channel_humid, (int)(g_humid), 0);
	
	//addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR,  "SHTC3: Temperature:%fC Humidity:%f%%", g_temp, g_humid);
}
// StopDriver SHTC3
void SHTC3_StopDriver() {
	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SHTC3: Stopping Driver and reset sensor");
	if (g_pin_power != -1) {
		HAL_PIN_SetOutputValue(g_pin_power&0x7F, ~(g_pin_power&0x80) >> 7); // power off
	}
}

void Soft_I2C_Init(void) {
    uint8_t i;
	/*
    //taskENTER_CRITICAL();
    i2c_user_setDC(1, 0);
    i2c_user_wait(i2s_clk_delay);
    // when SCL = 0, toggle SDA to clear up
    i2c_user_setDC(0, 0) ;
    i2c_user_wait(i2s_clk_delay);
    i2c_user_setDC(1, 0) ;
    i2c_user_wait(i2s_clk_delay);
    // set data_cnt to max value
    for (i = 0; i < 28; i++) {
        i2c_user_setDC(1, 0);
        i2c_user_wait(i2s_clk_delay); // sda 1, scl 0
        i2c_user_setDC(1, 1);
        i2c_user_wait(i2s_clk_delay); // sda 1, scl 1
    }
    //taskEXIT_CRITICAL();
    // reset all
    i2c_user_stop();
	*/
    return;
}

static commandResult_t SHTC3_SetErrorOutput(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	const char *error_channel_str = Tokenizer_GetArg(0);
	if (!error_channel_str) return CMD_RES_NOT_ENOUGH_ARGUMENTS;

	if ((error_channel_str[0] == 'C') && (error_channel_str[1] == 'H')) {
		g_error_channel = atoi(error_channel_str+2);
		int ch_type = CHANNEL_GetType(g_error_channel);
		if ((ch_type == ChType_Error) || (ch_type == ChType_Default)) {
			CHANNEL_SetType(g_error_channel, ChType_Error);
		} else {
			addLogAdv(LOG_WARN, LOG_FEATURE_SENSOR, "SHTC3: Error channel has not appropriet type"); 
		}
	}

	return CMD_RES_OK;
}
// startDriver SHTC3
void SHTC3_Init() {

	g_softI2C.pin_clk = 4;
	g_softI2C.pin_data = 5;
	g_pin_power = -1; // no power control

	g_softI2C.pin_clk = PIN_FindPinIndexForRole(IOR_SHTC3_CLK, g_softI2C.pin_clk);
	g_softI2C.pin_data = PIN_FindPinIndexForRole(IOR_SHTC3_DAT, g_softI2C.pin_data);
	if (g_pin_power == -1) {
		g_pin_power = PIN_FindPinIndexForRole(IOR_SHTC3_PWR, g_pin_power);	
		g_pin_power |= 0x80;
	}
	if (g_pin_power == -1) {
		g_pin_power = PIN_FindPinIndexForRole(IOR_SHTC3_PWR_n, g_pin_power);
		g_pin_power &= 0x7F;
	}
	
	if (g_pin_power != -1) {
		HAL_PIN_Setup_Output(g_pin_power&0x7F);		
		HAL_PIN_SetOutputValue(g_pin_power&0x7F, ~(g_pin_power&0x80) >> 7); // power off
	}

	Soft_I2C_PreInit(&g_softI2C);

	CMD_ExecuteCommand("SoftI2C_SetClkPeriod 250", COMMAND_FLAG_SOURCE_DRIVER);

	if (g_pin_power != -1) {
		HAL_PIN_SetOutputValue(g_pin_power&0x7F,  (g_pin_power&0x80) >> 7); // power on
	}
	rtos_delay_milliseconds(100);
	shtc3_soft_reset();
	if (g_pin_power != -1) {
		HAL_PIN_SetOutputValue(g_pin_power&0x7F, ~(g_pin_power&0x80) >> 7); // power off
	}	
	
	channel_temp = g_cfg.pins.channels[g_softI2C.pin_data];
	channel_humid = g_cfg.pins.channels2[g_softI2C.pin_data];
	
	CHANNEL_SetType(channel_temp, ChType_Temperature_div10);
	CHANNEL_SetType(channel_humid, ChType_Humidity);
	
	//cmddetail:{"name":"SHTC3_SetErrorOutput","args":"CHANNEL",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"SHTC3_SetErrorOutput","file":"drivers/drv_shtc3.c","requires":"",
	//cmddetail:"examples":"SHTC3_SetErrorOutput CH10"}
    CMD_RegisterCommand("SHTC3_SetErrorOutput", SHTC3_SetErrorOutput, NULL);
	
	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SHTC3: Init done (SCL=IO%d, SDA=IO%d, PWR=IO%d)" , 
	                                         HAL_GetGPIOPin(g_softI2C.pin_clk), 
											 HAL_GetGPIOPin(g_softI2C.pin_data),
											 HAL_GetGPIOPin(g_pin_power&0x7F));
}
void SHTC3_OnEverySecond()
{
	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SHTC3: Measure");
    SHTC3_Measure();
    /*
	if (g_sht_secondsUntilNextMeasurement <= 0) {
		if (g_shtper)
		{
			SHTC3_MeasurePercmd();
		}
		else
		{
			SHTC3_Measurecmd();
		}
		g_sht_secondsUntilNextMeasurement = g_sht_secondsBetweenMeasurements;
	}
	if (g_sht_secondsUntilNextMeasurement > 0) {
		g_sht_secondsUntilNextMeasurement--;
	}
	*/
}

void SHTC3_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState)
{
	if(bPreState)
		return;
	hprintf255(request, "<h2>SHTC3 Temperature=%.1fC, Humidity=%.0f%%</h2>", g_temp, g_humid);
	if (channel_humid == channel_temp) {
		hprintf255(request, "WARNING: You don't have configured target channels for temp and humid results, set the first and second channel index in Pins!");
	}
}

/**
 * @brief Function to get the device ID
 */
int shtc3_get_id(uint16_t *id)
{
	/* Variable to return error code */
	int ret = 0;

	shtc3_reg_write(SHTC3_CMD_READ_ID);

	uint8_t data[3] = {0};
	shtc3_reg_read(data, 3);

	/* Check data received CRC */
	if (!check_crc(data, 2, data[2])) {
		return -1;
	}

	*id = data[0] << 8 | data[1];

	/* Return 0 */
	return ret;
}
/**
 * @brief Function to get the temperature (°C) and humidity (%)
 */
int shtc3_get_temp_and_hum(float *temp, float *hum)
{
	/* Variable to return error code */
	int ret = 0;

	shtc3_wakeup();

	shtc3_reg_write(SHTC3_CMD_MEAS_T_RH_CLOCKSTR_NM);
	
	rtos_delay_milliseconds(300);

	uint8_t data[6] = {0};
	shtc3_reg_read(data, 6);

	/* Check data received CRC */
	if (!check_crc(&data[3], 2, data[5])) {
		return -1;
	}

	*temp = calc_temp((uint16_t)((data[0] << 8) | (data[1])));
	*hum = calc_hum((uint16_t)((data[3] << 8) | (data[4])));

	/* Return 0 */
	return ret;
}
 /**
 * @brief Function to get the temperature (°C) and humidity (%) in low
 *        power mode
 */
int shtc3_get_temp_and_hum_lpm(float *temp, float *hum)
{
	/* Variable to return error code */
	int ret = 0;

	//shtc3_wakeup();

	shtc3_reg_write(SHTC3_CMD_MEAS_T_RH_CLOCKSTR_LPM);

	rtos_delay_milliseconds(2);

	uint8_t data[6] = {0};
	shtc3_reg_read(data, 6);

	/* Check data received CRC */	
	if (!check_crc(&data[3], 2, data[5])) {
		return -1;
	}

	*temp = calc_temp((uint16_t)((data[0] << 8) | (data[1])));
	*hum = calc_hum((uint16_t)((data[3] << 8) | (data[4])));

	/* Return 0 */
	return ret;
}
/**
 * @brief Function to put the device in sleep mode
 */
int shtc3_sleep()
{
	/* Variable to return error code */
	int ret = 0;

	ret = shtc3_reg_write(SHTC3_CMD_SLEEP);

	/* Return 0 */
	return ret;
}
/**
 * @brief Function to wakeup the device from sleep mode
 */
int shtc3_wakeup()
{
	/* Variable to return error code */
	int ret = 0;

	ret = shtc3_reg_write(SHTC3_CMD_WAKEUP);
	rtos_delay_milliseconds(1);

	/* Return 0 */
	return ret;
}
/**
 * @brief Function to perfrom a software reset of the device
 */
int shtc3_soft_reset()
{
	/* Variable to return error code */
	int ret = 0;

	ret = shtc3_reg_write(SHTC3_CMD_SOFT_RESET);
	rtos_delay_milliseconds(1);

	/* Return 0 */
	return ret;
}

/* Private function definitions ----------------------------------------------*/
/**
 * @brief Function that implements the default I2C read transaction
 */
static int8_t shtc3_reg_read(uint8_t *data, uint32_t data_len)
{
	obk_enter_critical();
	//Transmit SHTC3 Address + read
	bool ack = Soft_I2C_Start(&g_softI2C, SHTC3_I2C_ADDR | 0x1);
	if(ack == false) {
		Soft_I2C_Stop(&g_softI2C);
		obk_exit_critical();
		return -1;
	}
	//Receive data
	Soft_I2C_ReadBytes(&g_softI2C, data, data_len);
	Soft_I2C_Stop(&g_softI2C);
	obk_exit_critical();
	return 0;
}
/**
 * @brief Function that implements the default I2C write transaction
 */
static int8_t shtc3_reg_write(uint16_t data)
{
	obk_enter_critical();
	//Transmit SHTC3 Address 	
	bool ack = Soft_I2C_Start(&g_softI2C, SHTC3_I2C_ADDR);
	if(ack == false) {
		Soft_I2C_Stop(&g_softI2C);
		obk_exit_critical();
		return -1;
	}
	//Send data
	ack = Soft_I2C_WriteByte(&g_softI2C, (uint8_t)((data >> 8) & 0xFF));
	if(ack == false) {
		Soft_I2C_Stop(&g_softI2C);
		obk_exit_critical();
		return -1;
	}	
	ack = Soft_I2C_WriteByte(&g_softI2C, (uint8_t)(data        & 0xFF)); 	
	if(ack == false) {
		Soft_I2C_Stop(&g_softI2C);
		obk_exit_critical();
		return -1;
	}
	Soft_I2C_Stop(&g_softI2C);
	obk_exit_critical();
	return 0;
}

/* Private macros ------------------------------------------------------------*/
#define CRC8_POLYNOMIAL	0x31
#define CRC8_INIT 		0xFF
#define CRC8_LEN 		1

/**
 * @brief Function that generates a CRC byte for a given data
 */
static uint8_t generate_crc(const uint8_t *data, uint16_t count) {
  uint16_t current_byte;
  uint8_t crc = CRC8_INIT;
  uint8_t crc_bit;

  /* calculates 8-Bit checksum with given polynomial */
  for (current_byte = 0; current_byte < count; ++current_byte) {
  	crc ^= (data[current_byte]);

  	for (crc_bit = 8; crc_bit > 0; --crc_bit) {
  		if (crc & 0x80) {
  			crc = (crc << 1) ^ CRC8_POLYNOMIAL;
  		}
  		else {
  			crc = (crc << 1);
  		}
  	}
  }
  return crc;
}

/**
 * @brief Function that checks the CRC for the received data
 */
static bool check_crc(const uint8_t *data, uint16_t count, uint8_t checksum) {
	if (generate_crc(data, count) != checksum) {
		return false;
	}

	return true;
}
/**
 * @brief Function that ...
 */
static float calc_temp(uint16_t raw_temp)
{
	return 175 * (float)raw_temp / 65536.0f - 45.0f;
}
/**
 * @brief Function that ...
 */
static float calc_hum(uint16_t raw_hum)
{
	return 100 * (float)raw_hum / 65536.0f;
}