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

#include "drv_sht3x.h"


#define SHT3X_I2C_ADDR (0x44 << 1)



// we'll need quite some testing for SHT3x sensor or return an error.
// keep memory low by using an general error message as const char*
static const char* ONLY_SHT3_ERROR_STR = "Only supported on SHT3x";

#define CHECK_SHT3X_SENSOR(CODE) do {                 \
    if (g_sensor_type != SENSOR_TYPE_SHT3X) {         \
        ADDLOG_ERROR(LOG_FEATURE_SENSOR,              \
            ONLY_SHT3_ERROR_STR);                     \
        return CODE;                                  \
    }                                                 \
} while (0)


static byte channel_temp = 0, channel_humid = 0, g_sht_secondsUntilNextMeasurement = 1, g_sht_secondsBetweenMeasurements = 10;
static float g_temp = 0.0, g_humid = 0.0, g_caltemp = 0.0, g_calhum = 0.0;
static bool g_shtper = false;
static softI2C_t g_softI2C;


// Sensor type enumeration - with GXHTV4 support
typedef enum {
	SENSOR_TYPE_UNKNOWN = 0,
	SENSOR_TYPE_SHT3X = 1,
	SENSOR_TYPE_SHT4X = 2,
	SENSOR_TYPE_GXHTV4 = 3
} sensor_type_t;

static sensor_type_t g_sensor_type = SENSOR_TYPE_UNKNOWN;  // Track sensor type

// Calculate CRC-8 for SHT3X/SHT4X (2 bytes at a time)
// replaces old CRC function using a pointer and working on p[0] and p[1]
static uint8_t SHT_CalcCrc(uint8_t byte1, uint8_t byte2) {
	uint8_t crc = 0xFF;
	
	// First byte
	crc ^= byte1;
	for (uint8_t j = 0; j < 8; j++) {
		if (crc & 0x80) {
			crc = (crc << 1) ^ 0x31;
		} else {
			crc = (crc << 1);
		}
	}
	
	// Second byte
	crc ^= byte2;
	for (uint8_t j = 0; j < 8; j++) {
		if (crc & 0x80) {
			crc = (crc << 1) ^ 0x31;
		} else {
			crc = (crc << 1);
		}
	}
	
	return crc;
}



// Try to distinguish SHT3X from SHT4X/GXHTV4 using CRC validation on serial
// Format: Word1: [SNB_3, SNB_2, CRC_word1] | Word2: [SNB_1, SNB_0, CRC_word2]
//
// no complete documentation for GXHTV4 available but some user feedback, that it differs from SHT4x regarding reading serial number with SHT3x command (0x36 0x82):
// SHT4x will send ACK for command 0x36 but NACK for 0x82 
// actual hypothesis: GXHTV4 returns constant serial "00000000" when using SHT3X get serial command for the first time, later uses a "fake" serial build
static sensor_type_t SHT_DetectSensorType() {
	uint8_t data[6];
	uint8_t crc_word1, crc_word2;
	bool is_GXHTV4 = false;
	
	ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT: Detecting sensor type...");
	
	rtos_delay_milliseconds(10);
	
	// TEST 1 : Try Serial Read command 0x89 (SHT4X or GXHTV4) - Also present on SHT3x as reset, but it should not deliver serial afterwards!
	Soft_I2C_Start(&g_softI2C, SHT3X_I2C_ADDR);
	bool serial_0x89_ack = Soft_I2C_WriteByte(&g_softI2C, 0x89);
	Soft_I2C_Stop(&g_softI2C);
	if (serial_0x89_ack) {
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "DEBUG SHT_DetectSensorType() testing for SHT4x/GXHTV4 serial: 0x89 got ACK");
		rtos_delay_milliseconds(2);
		
		Soft_I2C_Start(&g_softI2C, SHT3X_I2C_ADDR | 1);
		Soft_I2C_ReadBytes(&g_softI2C, data, 6);
		Soft_I2C_Stop(&g_softI2C);
		
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT: 0x89 cmd: SNB=[%02X%02X%02X%02X], CRC=[%02X,%02X]", 
			data[0], data[1], data[3], data[4], data[2], data[5]);
		
		// Verify CRC for 0x89 response
		crc_word1 = SHT_CalcCrc(data[0], data[1]);
		crc_word2 = SHT_CalcCrc(data[3], data[4]);
		
		// If CRCs are valid, it's either SHT4X or GXHTV4
		if (crc_word1 == data[2] && crc_word2 == data[5]) {
			ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT: ✓ Detected SHT4X/GXHTV4 (Valid CRC - Serial via 0x89: %02X%02X%02X%02X)", 
				data[0], data[1], data[3], data[4]);
			if (is_GXHTV4) return SENSOR_TYPE_GXHTV4;  	// Return GXHTV4
			return SENSOR_TYPE_SHT4X;  			// Return SHT4X
		}
	}


	rtos_delay_milliseconds(10);


	// TEST 2 : Test 1 did not succeed if we are here. Try SHT3X Serial Number Read (0x36 0x82 - no clock stretching)
	Soft_I2C_Start(&g_softI2C, SHT3X_I2C_ADDR);
	bool sht3x_byte1_ack = Soft_I2C_WriteByte(&g_softI2C, 0x36);
	bool sht3x_byte2_ack = false;
	
	if (sht3x_byte1_ack) {
		sht3x_byte2_ack = Soft_I2C_WriteByte(&g_softI2C, 0x82);
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "DEBUG SHT_DetectSensorType() testing for SHT3x: 0x36 got ACK, 0x82 got %s", sht3x_byte2_ack ? "ACK" : "NACK");
	} else {ADDLOG_INFO(LOG_FEATURE_SENSOR, "DEBUG SHT_DetectSensorType() testing for SHT3x: 0x36 got NACK");}
	Soft_I2C_Stop(&g_softI2C);
	
	if (sht3x_byte1_ack && sht3x_byte2_ack) {
		rtos_delay_milliseconds(2);
		
		Soft_I2C_Start(&g_softI2C, SHT3X_I2C_ADDR | 1);
		Soft_I2C_ReadBytes(&g_softI2C, data, 6);
		Soft_I2C_Stop(&g_softI2C);
		
		// Format: [SNB_3, SNB_2, CRC_word1, SNB_1, SNB_0, CRC_word2]
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT3X cmd: SNB=[%02X%02X%02X%02X], CRC=[%02X,%02X]", 
			data[0], data[1], data[3], data[4], data[2], data[5]);
		
		// Calculate CRC
		crc_word1 = SHT_CalcCrc(data[0], data[1]);  // CRC of SNB_3 and SNB_2
		crc_word2 = SHT_CalcCrc(data[3], data[4]);  // CRC of SNB_1 and SNB_0
		
		// Check for GXHTV4 constant dummy pattern
		// GXHTV4 returns: SNB_3=0x00, SNB_2=0x00, CRC1=0x81, SNB_1=0x00, SNB_0=0x00, CRC1=0x81
		if (data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x81 && data[3] == 0x00 && data[4] == 0x00 && data[5] == 0x81) {
			ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT: Detected serial \"00000000\" in SHT3X cmd - assuming GXHTV4)");
			is_GXHTV4 = true;
			// Will read 0x89 serial below for confirmation
		} else {
			// Check if CRCs match (valid real SHT3X)
			if (crc_word1 == data[2] && crc_word2 == data[5]) {
				ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT: ✓ Detected SHT3X (Valid CRC - Serial: %02X%02X%02X%02X)", 
					data[0], data[1], data[3], data[4]);
				return SENSOR_TYPE_SHT3X;
			}
		}
	}

	if (is_GXHTV4) return SENSOR_TYPE_GXHTV4;  	// not sure about CRC, so return GXHTV4 if we set it
	// Fallback default
	ADDLOG_WARN(LOG_FEATURE_SENSOR, "SHT: ⚠ Sensor detection failed, defaulting to SHT3X");
	return SENSOR_TYPE_SHT3X;
}


commandResult_t SHT3X_Calibrate(const void* context, const char* cmd, const char* args, int cmdFlags) {

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2))
	{
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_caltemp = Tokenizer_GetArgFloat(0);
	g_calhum = Tokenizer_GetArgFloat(1);

	ADDLOG_INFO(LOG_FEATURE_SENSOR, "Calibrate SHT: Calibration done temp %f and humidity %f ", g_caltemp, g_calhum);

	return CMD_RES_OK;
}

void SHT3X_StopPer() {
	// test is SHT3x or return logging the error
	CHECK_SHT3X_SENSOR();
	Soft_I2C_Start(&g_softI2C, SHT3X_I2C_ADDR);
	// Stop Periodic Data
	Soft_I2C_WriteByte(&g_softI2C, 0x30);
	// medium repeteability
	Soft_I2C_WriteByte(&g_softI2C, 0x93);
	Soft_I2C_Stop(&g_softI2C);
	g_shtper = false;
}

void SHT3X_StartPer(uint8_t msb, uint8_t lsb) {
	// test is SHT3x or return logging the error
	CHECK_SHT3X_SENSOR();
	// Start Periodic Data capture
	Soft_I2C_Start(&g_softI2C, SHT3X_I2C_ADDR);
	// Measure per seconds
	Soft_I2C_WriteByte(&g_softI2C, msb);
	// repeteability
	Soft_I2C_WriteByte(&g_softI2C, lsb);
	Soft_I2C_Stop(&g_softI2C);
	g_shtper = true;
}

commandResult_t SHT3X_ChangePer(const void* context, const char* cmd, const char* args, int cmdFlags) {
	// test is SHT3x or return error
	CHECK_SHT3X_SENSOR(CMD_RES_ERROR);
	uint8_t g_msb, g_lsb;
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if (Tokenizer_GetArgsCount() < 2) {
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT Change Per: no arg using default0x23 0x22");
		g_msb = 0x23;
		g_lsb = 0x22;
	}
	else {
		g_msb = Tokenizer_GetArgInteger(0);
		g_lsb = Tokenizer_GetArgInteger(1);
	}
	SHT3X_StopPer();
	//give some time for SHT to stop Periodicity
	rtos_delay_milliseconds(25);
	SHT3X_StartPer(g_msb, g_lsb);

	ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT Change Per : change done");

	return CMD_RES_OK;

}

// only SHT3 heater is switched on / off by command
//
// SHT4x only has "timed heating with power X and measuring on deactivation" 
// e.g. command 0x32: "activate heater with 200mW for 1s, including a high precision measurement just before deactivation"
//  or command  0x15: "activate heater with 20mW for 0.1s including a high precision measurement just before deactivation"
//
commandResult_t SHT3X_Heater(const void* context, const char* cmd, const char* args, int cmdFlags) {
	// test is SHT3x or return logging the error
	CHECK_SHT3X_SENSOR(CMD_RES_ERROR);
	int g_state_heat;
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		//ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT Heater: 1 or 0 to activate");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_state_heat = Tokenizer_GetArgInteger(0);
	Soft_I2C_Start(&g_softI2C, SHT3X_I2C_ADDR);
	// medium repeteability
	Soft_I2C_WriteByte(&g_softI2C, 0x30);

	if (g_state_heat > 0) {
		Soft_I2C_WriteByte(&g_softI2C, 0x6D);
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT Heater activated");
	}
	else {
		Soft_I2C_WriteByte(&g_softI2C, 0x66);
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT Heater deactivated");
	}
	Soft_I2C_Stop(&g_softI2C);
	return CMD_RES_OK;
}

void SHT3X_MeasurePercmd() {
	// test is SHT3x or return logging the error
	CHECK_SHT3X_SENSOR();
#if WINDOWS
	// TODO: values for simulator so I can test SHT30 
	// on my Windows machine
	g_temp = 23.4f;
	g_humid = 56.7f;
#else

	uint8_t buff[6];
	unsigned int th, tl, hh, hl;

	Soft_I2C_Start(&g_softI2C, SHT3X_I2C_ADDR);
	// Ask for fetching data
	Soft_I2C_WriteByte(&g_softI2C, 0xE0);
	// medium repeteability
	Soft_I2C_WriteByte(&g_softI2C, 0x00);
	Soft_I2C_Stop(&g_softI2C);

	Soft_I2C_Start(&g_softI2C, SHT3X_I2C_ADDR | 1);
	Soft_I2C_ReadBytes(&g_softI2C, buff, 6);
	Soft_I2C_Stop(&g_softI2C);

	th = buff[0];
	tl = buff[1];
	hh = buff[3];
	hl = buff[4];

	g_temp = 175 * ((th * 256 + tl) / 65535.0) - 45.0;
	g_humid = 100 * ((hh * 256 + hl) / 65535.0);
#endif

	g_temp = (int)((g_temp + g_caltemp) * 10.0) / 10.0f;
	g_humid = (int)(g_humid + g_calhum);

	channel_temp = g_cfg.pins.channels[g_softI2C.pin_data];
	channel_humid = g_cfg.pins.channels2[g_softI2C.pin_data];
	CHANNEL_Set(channel_temp, (int)(g_temp * 10), 0);
	CHANNEL_Set(channel_humid, (int)(g_humid), 0);

	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SHT3X_Measure: Period Temperature:%.2fC Humidity:%.0f%%", g_temp, g_humid);
}

commandResult_t SHT3X_MeasurePer(const void* context, const char* cmd, const char* args, int cmdFlags) {
	// test is SHT3x or return logging the error
	CHECK_SHT3X_SENSOR(CMD_RES_ERROR);
	SHT3X_MeasurePercmd();
	return CMD_RES_OK;
}

void SHT3X_Measurecmd() {
#if WINDOWS
	// TODO: values for simulator so I can test SHT30 
	// on my Windows machine
	g_temp = 23.4f;
	g_humid = 56.7f;
	// no real sensor, no CRC check, no possible CRC mismatch
	const char errstr[] = "";
#else
	uint8_t buff[6];
//	uint8_t th, tl, hh, hl,crc1,crc2;
	uint8_t crc1,crc2;

	Soft_I2C_Start(&g_softI2C, SHT3X_I2C_ADDR);
	// MODIFIED: Sensor-specific measurement commands
	if (g_sensor_type == SENSOR_TYPE_SHT4X || g_sensor_type == SENSOR_TYPE_GXHTV4) {
		// todo: maybe make this configurable ?!?
		// SHT4X/GXHTV4: High precision measurement
//		Soft_I2C_WriteByte(&g_softI2C, 0xFD);  // high precision
		// SHT4X/GXHTV4: Low precision measurement
		// for now use LP, as battery powered GXHTV4 device does
		Soft_I2C_WriteByte(&g_softI2C, 0xE0);  // low precision
		addLogAdv(LOG_DEBUG, LOG_FEATURE_SENSOR, "SHT3X : Wrote SHT4x/GXHTV4 measure command 0xE0");
	} else {
		// no clock stretching
		Soft_I2C_WriteByte(&g_softI2C, 0x24);
		// medium repeteability
		Soft_I2C_WriteByte(&g_softI2C, 0x16);
		addLogAdv(LOG_DEBUG, LOG_FEATURE_SENSOR, "SHT3X : Wrote SHT3X measure command 0x24 0x16");
	}
	Soft_I2C_Stop(&g_softI2C);

	rtos_delay_milliseconds(30);	//give the sensor time to do the conversion

	Soft_I2C_Start(&g_softI2C, SHT3X_I2C_ADDR | 1);
	Soft_I2C_ReadBytes(&g_softI2C, buff, 6);
	Soft_I2C_Stop(&g_softI2C);
	
	crc1=SHT_CalcCrc(buff[0], buff[1]);
	crc2=SHT_CalcCrc(buff[3], buff[4]);

	
	// test for crc - build error string in case of mismatch
	char errstr[128] = {0};

	if (crc1 != buff[2] || crc2 != buff[5]) {
	    int offset = 0;
	    offset += snprintf(errstr + offset, sizeof(errstr) - offset, "  ! CRC mismatch read(calculated)!");
	    
	    if (crc1 != buff[2]) {
		offset += snprintf(errstr + offset, sizeof(errstr) - offset, 
		                  " Temp CRC 0X%02X(0X%02X).", buff[2], crc1);
	    }
	    if (crc2 != buff[5]) {
		offset += snprintf(errstr + offset, sizeof(errstr) - offset, 
		                  " Hum CRC 0X%02X(0X%02X).", buff[5], crc2);
	    }
	}

	addLogAdv(LOG_DEBUG, LOG_FEATURE_SENSOR, "SHT3X : Read: temp=0X%02X%02X (CRC1=0X%02X) hum=0X%02X%02X (CRC2=0X%02X).%s",
		buff[0], buff[1], buff[2],
		buff[3],buff[4], buff[5],
		errstr
		);
	if (g_sensor_type == SENSOR_TYPE_GXHTV4) {
		addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "GXHTV4 - try CHTX style calculation: Temp=%.2f - Hum=%.2f",
		(float)((buff[0] << 8 | buff[1]) * 165.0 / 65535.0 - 40.0),
		(float)((buff[2] << 8 | buff[3]) * 100.0 / 65535.0)
		);
	}

	g_temp = 175 * ((buff[0] << 8 | buff[1]) / 65535.0) - 45.0;

	// Different humidity formulas
	// hum is (buff[3] * 256 + buff[4]) or (buff[3] << 8 | buff[4])
	//
	// SHT3X:			RH = 100 * hum / (2^16 -1) 	= hum / ((2^16 -1) /100) = hum / 655.35
	// SHT4x/GXHTV4:		RH = 125 * hum / (2^16 -1) - 6	= 1.25 * (100 * hum / (2^16 -1)) - 6 = 1.25 * hum / 655.35 - 6
	// 
	g_humid = ((buff[3] << 8 | buff[4]) / 655.35);	// start with common term, final value for SHT3X
	// RH = -6 + 125 * hum / (2^16)
	if (g_sensor_type != SENSOR_TYPE_SHT3X) {
		g_humid *= 1.25;
		g_humid -= 6;
		if (g_humid > 100.0) g_humid = 100.0;
		if (g_humid < 0.0) g_humid = 0.0;
	}

#endif

	g_temp = (int)((g_temp + g_caltemp) * 10.0) / 10.0f;
	g_humid = (int)(g_humid + g_calhum);

	channel_temp = g_cfg.pins.channels[g_softI2C.pin_data];
	channel_humid = g_cfg.pins.channels2[g_softI2C.pin_data];
	CHANNEL_Set(channel_temp, (int)(g_temp * 10), 0);
	CHANNEL_Set(channel_humid, (int)(g_humid), 0);

	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, 
		  "SHT3X_Measure: Temperature:%.1fC Humidity:%.0f%%%s", 
		  g_temp, g_humid, errstr);

}

commandResult_t SHT3X_Measure(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	SHT3X_Measurecmd();
	return CMD_RES_OK;
}
// StopDriver SHT3X
void SHT3X_StopDriver() {
	// Sensor-specific reset commands
	if (g_sensor_type == SENSOR_TYPE_SHT4X || g_sensor_type == SENSOR_TYPE_GXHTV4) {
		addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SHT4X/GXHTV4: Stopping Driver and reset sensor");
		SHT3X_StopPer();
		Soft_I2C_Start(&g_softI2C, SHT3X_I2C_ADDR);
		Soft_I2C_WriteByte(&g_softI2C, 0x09);
		Soft_I2C_Stop(&g_softI2C);
	} else {
		addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SHT3X : Stopping Driver and reset sensor");
		SHT3X_StopPer();
		// Reset the sensor
		Soft_I2C_Start(&g_softI2C, SHT3X_I2C_ADDR);
		Soft_I2C_WriteByte(&g_softI2C, 0x30);
		Soft_I2C_WriteByte(&g_softI2C, 0xA2);
		Soft_I2C_Stop(&g_softI2C);
	}
}

commandResult_t SHT3X_StopPerCmd(const void* context, const char* cmd, const char* args, int cmdFlags) {
	// test is SHT3x or return logging the error
	CHECK_SHT3X_SENSOR(CMD_RES_ERROR);
	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SHT3X : Stopping periodical capture");
	SHT3X_StopPer();
	return CMD_RES_OK;
}

void SHT3X_GetStatus()
{
	// test is SHT3x or return logging the error
	CHECK_SHT3X_SENSOR();
	uint8_t status[2];
	Soft_I2C_Start(&g_softI2C, SHT3X_I2C_ADDR);
	Soft_I2C_WriteByte(&g_softI2C, 0xf3);			//Get Status should be 00000xxxxx00x0x0
	Soft_I2C_WriteByte(&g_softI2C, 0x2d);          //Cheksum/Cmd_status/x/reset/res*5/Talert/RHalert/x/Heater/x/Alert
	Soft_I2C_Stop(&g_softI2C);
	Soft_I2C_Start(&g_softI2C, SHT3X_I2C_ADDR | 1);
	Soft_I2C_ReadBytes(&g_softI2C, status, 2);
	Soft_I2C_Stop(&g_softI2C);
	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SHT : Status : %02X %02X", status[0], status[1]);
}
commandResult_t SHT3X_GetStatusCmd(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	// test is SHT3x or return logging the error
	CHECK_SHT3X_SENSOR(CMD_RES_ERROR);
	SHT3X_GetStatus();
	return CMD_RES_OK;
}
void SHT3X_ClearStatus()
{
	// test is SHT3x or return logging the error
	CHECK_SHT3X_SENSOR();
	Soft_I2C_Start(&g_softI2C, SHT3X_I2C_ADDR);
	// MODIFIED: Sensor-specific clear status commands
	if (g_sensor_type == SENSOR_TYPE_SHT4X || g_sensor_type == SENSOR_TYPE_GXHTV4) {
		Soft_I2C_WriteByte(&g_softI2C, 0x3C);
		Soft_I2C_WriteByte(&g_softI2C, 0x84);
	} else {
		Soft_I2C_WriteByte(&g_softI2C, 0x30);			//Clear status
		Soft_I2C_WriteByte(&g_softI2C, 0x41);          //clear status
	}
	Soft_I2C_Stop(&g_softI2C);
	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SHT : Clear status");
}
commandResult_t SHT3X_ClearStatusCmd(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	// test is SHT3x or return logging the error
	CHECK_SHT3X_SENSOR(CMD_RES_ERROR);
	SHT3X_ClearStatus();
	return CMD_RES_OK;
}
// Alert management for SHT3X
static void SHT3X_ReadAlertLimitData(float* humidity, float* temperature)
{
	// test is SHT3x or return logging the error
	CHECK_SHT3X_SENSOR();
	uint8_t data[2];
	uint16_t finaldata;
	Soft_I2C_Start(&g_softI2C, SHT3X_I2C_ADDR | 1);
	Soft_I2C_ReadBytes(&g_softI2C, data, 2);
	Soft_I2C_Stop(&g_softI2C);
	ADDLOG_DEBUG(LOG_FEATURE_SENSOR, "SHT Get alert: Reading below value %02X %02X", data[0], data[1]);
	finaldata = (data[0] << 8) | data[1];
	(*humidity) = 100.0f * (finaldata & 0xFE00) / 65535.0f;
	finaldata = finaldata << 7;
	ADDLOG_DEBUG(LOG_FEATURE_SENSOR, "SHT Get alert: Reading below final value %04X", finaldata);
	(*temperature) = (175.0f * finaldata / 65535.0f) - 45.0f;
}

/*
// use SHT_CalcCrc(uint8_t byte1, uint8_t byte2)
static uint8_t SHT3X_CalcCrc(uint8_t* data)
{
	uint8_t bit;        // bit mask
	uint8_t crc = 0xFF; // calculated checksum

	// calculates 8-Bit checksum with given polynomial
	crc ^= (data[0]);
	for (bit = 8; bit > 0; --bit)
	{
		if ((crc & 0x80))
		{
			crc = (crc << 1) ^ 0x131;
		}
		else
		{
			crc = (crc << 1);
		}
	}

	crc ^= (data[1]);
	for (bit = 8; bit > 0; --bit)
	{
		if ((crc & 0x80))
		{
			crc = (crc << 1) ^ 0x131;
		}
		else
		{
			crc = (crc << 1);
		}
	}

	return crc;
}
*/
void SHT3X_WriteAlertLimitData(float humidity, float temperature)
{
	// test is SHT3x or return logging the error
	CHECK_SHT3X_SENSOR();
	uint8_t data[2], checksum;
	uint16_t finaldata;
	uint16_t rawHumidity, rawTemperature;
	if ((humidity < 0.0f) || (humidity > 100.0f)
		|| (temperature < -45.0f) || (temperature > 130.0f))
	{
		ADDLOG_INFO(LOG_FEATURE_CMD, "SHT Set Alert: Value out of limit");
	}
	else
	{
		rawHumidity = humidity / 100.0f * 65535.0f;
		rawTemperature = (temperature + 45.0f) / 175.0f * 65535.0f;
		ADDLOG_DEBUG(LOG_FEATURE_SENSOR, "SHT Set alert: Raw Value temp/hum %02X %02X ", rawTemperature, rawHumidity);
		finaldata = (rawHumidity & 0xFE00) | ((rawTemperature >> 7) & 0x001FF);
		data[0] = finaldata >> 8;
		data[1] = finaldata & 0xFF;
		checksum = SHT_CalcCrc(data[0],data[1]);
		Soft_I2C_WriteByte(&g_softI2C, data[0]);
		Soft_I2C_WriteByte(&g_softI2C, data[1]);
		Soft_I2C_WriteByte(&g_softI2C, checksum);
		ADDLOG_DEBUG(LOG_FEATURE_SENSOR, "SHT Set alert: writing below value %02X %02X %02X", data[0], data[1], checksum);
	}
}

void SHT3X_GetAlertLimits()
{
	// test is SHT3x or return logging the error
	CHECK_SHT3X_SENSOR();

	float temperatureLowSet, temperatureLowClear, temperatureHighClear, temperatureHighSet;
	float humidityLowSet, humidityLowClear, humidityHighClear, humidityHighSet;
	// read humidity & temperature alter limits, high set
	Soft_I2C_Start(&g_softI2C, SHT3X_I2C_ADDR);
	Soft_I2C_WriteByte(&g_softI2C, 0xe1);
	Soft_I2C_WriteByte(&g_softI2C, 0x1f);
	Soft_I2C_Stop(&g_softI2C);
	SHT3X_ReadAlertLimitData(&humidityHighSet, &temperatureHighSet);

	Soft_I2C_Start(&g_softI2C, SHT3X_I2C_ADDR);
	Soft_I2C_WriteByte(&g_softI2C, 0xe1);
	Soft_I2C_WriteByte(&g_softI2C, 0x14);
	Soft_I2C_Stop(&g_softI2C);
	SHT3X_ReadAlertLimitData(&humidityHighClear, &temperatureHighClear);

	Soft_I2C_Start(&g_softI2C, SHT3X_I2C_ADDR);
	Soft_I2C_WriteByte(&g_softI2C, 0xe1);
	Soft_I2C_WriteByte(&g_softI2C, 0x09);
	Soft_I2C_Stop(&g_softI2C);
	SHT3X_ReadAlertLimitData(&humidityLowClear, &temperatureLowClear);

	Soft_I2C_Start(&g_softI2C, SHT3X_I2C_ADDR);
	Soft_I2C_WriteByte(&g_softI2C, 0xe1);
	Soft_I2C_WriteByte(&g_softI2C, 0x02);
	Soft_I2C_Stop(&g_softI2C);
	SHT3X_ReadAlertLimitData(&humidityLowSet, &temperatureLowSet);

	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SHT : Read Alert conf _ Temp : %f / %f / %f / %f ", temperatureLowSet, temperatureLowClear, temperatureHighClear, temperatureHighSet);
	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SHT : Read Alert conf _ Hum : %f / %f / %f / %f ", humidityLowSet, humidityLowClear, humidityHighClear, humidityHighSet);
}
commandResult_t SHT3X_ReadAlertCmd(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	// test is SHT3x or return logging the error
	CHECK_SHT3X_SENSOR(CMD_RES_ERROR);
	SHT3X_GetAlertLimits();
	return CMD_RES_OK;
}

commandResult_t SHT3X_SetAlertCmd(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	// test is SHT3x or return logging the error
	CHECK_SHT3X_SENSOR(CMD_RES_ERROR);

	float temperatureLowSet, temperatureLowClear, temperatureHighClear, temperatureHighSet;
	float humidityLowSet, humidityLowClear, humidityHighClear, humidityHighSet;
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 4)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	//ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT Set alert: require Temp low/high and Humidity low/high arg");
	temperatureHighSet = Tokenizer_GetArgFloat(0);
	temperatureLowSet = Tokenizer_GetArgFloat(1);
	temperatureLowClear = Tokenizer_GetArgFloat(1) + 0.5;
	temperatureHighClear = Tokenizer_GetArgFloat(0) - 0.5;

	humidityHighSet = Tokenizer_GetArgFloat(2);
	humidityLowSet = Tokenizer_GetArgFloat(3);
	humidityLowClear = Tokenizer_GetArgFloat(3) + 2.0f;
	humidityHighClear = Tokenizer_GetArgFloat(2) - 2.0f;

	Soft_I2C_Start(&g_softI2C, SHT3X_I2C_ADDR);
	Soft_I2C_WriteByte(&g_softI2C, 0x61);
	Soft_I2C_WriteByte(&g_softI2C, 0x1d);
	SHT3X_WriteAlertLimitData(humidityHighSet, temperatureHighSet);
	Soft_I2C_Stop(&g_softI2C);

	Soft_I2C_Start(&g_softI2C, SHT3X_I2C_ADDR);
	Soft_I2C_WriteByte(&g_softI2C, 0x61);
	Soft_I2C_WriteByte(&g_softI2C, 0x16);
	SHT3X_WriteAlertLimitData(humidityHighClear, temperatureHighClear);
	Soft_I2C_Stop(&g_softI2C);

	Soft_I2C_Start(&g_softI2C, SHT3X_I2C_ADDR);
	Soft_I2C_WriteByte(&g_softI2C, 0x61);
	Soft_I2C_WriteByte(&g_softI2C, 0x0B);
	SHT3X_WriteAlertLimitData(humidityLowClear, temperatureLowClear);
	Soft_I2C_Stop(&g_softI2C);

	Soft_I2C_Start(&g_softI2C, SHT3X_I2C_ADDR);
	Soft_I2C_WriteByte(&g_softI2C, 0x61);
	Soft_I2C_WriteByte(&g_softI2C, 0x00);
	SHT3X_WriteAlertLimitData(humidityLowSet, temperatureLowSet);
	Soft_I2C_Stop(&g_softI2C);

	ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT: set alert for temp %f / %f and humidity %f / %f ", temperatureLowSet, temperatureHighSet, humidityLowSet, humidityHighSet);

	return CMD_RES_OK;
}
commandResult_t SHT_cycle(const void* context, const char* cmd, const char* args, int cmdFlags) {

	// test is SHT3x or return logging the error
	CHECK_SHT3X_SENSOR(CMD_RES_ERROR);
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_sht_secondsBetweenMeasurements = Tokenizer_GetArgInteger(0);

	ADDLOG_INFO(LOG_FEATURE_CMD, "Measurement will run every %i seconds", g_sht_secondsBetweenMeasurements);

	return CMD_RES_OK;
}

// NEW: Command to set sensor type manually
commandResult_t SHT_SetSensorType(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int sensor_type;
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	sensor_type = Tokenizer_GetArgInteger(0);
	
	if (sensor_type == 3) {
		g_sensor_type = SENSOR_TYPE_SHT3X;
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "Sensor type set to SHT3X");
	} else if (sensor_type == 4 ) {
		g_sensor_type = SENSOR_TYPE_SHT4X;
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "Sensor type set to SHT4X");
	} else if (sensor_type == 5 ) {
		g_sensor_type = SENSOR_TYPE_GXHTV4;
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "Sensor type set to GXHTV4");
	} else {
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "Invalid sensor type. Use 3 for SHT3X, 4 for SHT4X or 5 for GXHTV4");
		return CMD_RES_BAD_ARGUMENT;
	}
	
	return CMD_RES_OK;
}

// startDriver SHT3X
void SHT3X_Init() {
	g_softI2C.pin_clk = 9;
	g_softI2C.pin_data = 14;

	g_softI2C.pin_clk = PIN_FindPinIndexForRole(IOR_SHT3X_CLK, g_softI2C.pin_clk);
	g_softI2C.pin_data = PIN_FindPinIndexForRole(IOR_SHT3X_DAT, g_softI2C.pin_data);

	Soft_I2C_PreInit(&g_softI2C);
	ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT3x: using pins %i (SCL) and %i (SDA)",g_softI2C.pin_clk,g_softI2C.pin_data);

	// NEW: Auto-detect sensor type
	g_sensor_type = SHT_DetectSensorType();
	if (g_sensor_type == SENSOR_TYPE_SHT4X) {
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT4X sensor detected");
	} else {
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT3X sensor detected");
	}

	SHT3X_GetStatus();

	//cmddetail:{"name":"SHT_cycle","args":"[int]",
	//cmddetail:"descr":"This is the interval between measurements in seconds, by default 10. Max is 255.",
	//cmddetail:"fn":"SHT_cycle","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_Cycle 60"}
	CMD_RegisterCommand("SHT_cycle", SHT_cycle, NULL);
	
	//cmddetail:{"name":"SHT_SetType","args":"[type]",
	//cmddetail:"descr":"Set sensor type: 3=SHT3X, 4=SHT4X, 5=GXHTV4",
	//cmddetail:"fn":"SHT_SetSensorType","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_SetType 4"}
	CMD_RegisterCommand("SHT_SetType", SHT_SetSensorType, NULL);
	
	//cmddetail:{"name":"SHT_Calibrate","args":"[DeltaTemp][DeltaHumidity]",
	//cmddetail:"descr":"Calibrate the SHT Sensor as Tolerance is +/-2 degrees C.",
	//cmddetail:"fn":"SHT3X_Calibrate","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_Calibrate -4 10"}
	CMD_RegisterCommand("SHT_Calibrate", SHT3X_Calibrate, NULL);
	//cmddetail:{"name":"SHT_MeasurePer","args":"",
	//cmddetail:"descr":"Retrieve Periodical measurement for SHT",
	//cmddetail:"fn":"SHT3X_MeasurePer","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_Measure"}
	CMD_RegisterCommand("SHT_MeasurePer", SHT3X_MeasurePer, NULL);
	//cmddetail:{"name":"SHT_LaunchPer","args":"[msb][lsb]",
	//cmddetail:"descr":"Launch/Change periodical capture for SHT Sensor",
	//cmddetail:"fn":"SHT3X_ChangePer","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_LaunchPer 0x23 0x22"}
	CMD_RegisterCommand("SHT_LaunchPer", SHT3X_ChangePer, NULL);
	//cmddetail:{"name":"SHT_StopPer","args":"",
	//cmddetail:"descr":"Stop periodical capture for SHT Sensor",
	//cmddetail:"fn":"SHT3X_StopPerCmd","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SHT_StopPer", SHT3X_StopPerCmd, NULL);
	//cmddetail:{"name":"SHT_Measure","args":"",
	//cmddetail:"descr":"Retrieve OneShot measurement for SHT",
	//cmddetail:"fn":"SHT3X_Measure","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_Measure"}
	CMD_RegisterCommand("SHT_Measure", SHT3X_Measure, NULL);
	//cmddetail:{"name":"SHT_Heater","args":"[1or0]",
	//cmddetail:"descr":"Activate or Deactivate Heater (0 / 1)",
	//cmddetail:"fn":"SHT3X_Heater","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_Heater 1"}
	CMD_RegisterCommand("SHT_Heater", SHT3X_Heater, NULL);
	//cmddetail:{"name":"SHT_GetStatus","args":"",
	//cmddetail:"descr":"Get Sensor Status",
	//cmddetail:"fn":"SHT3X_GetStatusCmd","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_GetStatusCmd"}
	CMD_RegisterCommand("SHT_GetStatus", SHT3X_GetStatusCmd, NULL);
	//cmddetail:{"name":"SHT_ClearStatus","args":"",
	//cmddetail:"descr":"Clear Sensor Status",
	//cmddetail:"fn":"SHT3X_ClearStatusCmd","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_ClearStatusCmd"}
	CMD_RegisterCommand("SHT_ClearStatus", SHT3X_ClearStatusCmd, NULL);
	//cmddetail:{"name":"SHT_ReadAlert","args":"",
	//cmddetail:"descr":"Get Sensor alert configuration - only for SHT3x sensors",
	//cmddetail:"fn":"SHT3X_ReadAlertCmd","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_ReadAlertCmd"}
	CMD_RegisterCommand("SHT_ReadAlert", SHT3X_ReadAlertCmd, NULL);
	//cmddetail:{"name":"SHT_SetAlert","args":"[temp_high, temp_low, hum_high, hum_low]",
	//cmddetail:"descr":"Set Sensor alert configuration - only for SHT3x sensors",
	//cmddetail:"fn":"SHT3X_SetAlertCmd","file":"driver/drv_sht3x.c","requires":"all",
	//cmddetail:"examples":"SHT_SetAlertCmd"}
	CMD_RegisterCommand("SHT_SetAlert", SHT3X_SetAlertCmd, NULL);
}
void SHT3X_OnEverySecond()
{

	if (g_sht_secondsUntilNextMeasurement <= 0) {
		if (g_shtper)
		{
			SHT3X_MeasurePercmd();
		}
		else
		{
			SHT3X_Measurecmd();
		}
		g_sht_secondsUntilNextMeasurement = g_sht_secondsBetweenMeasurements;
	}
	if (g_sht_secondsUntilNextMeasurement > 0) {
		g_sht_secondsUntilNextMeasurement--;
	}
}

void SHT3X_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState)
{
	if(bPreState)
		return;
	// NEW: Show which sensor type is detected
	const char* sensor_name = "SHT3X";
	if (g_sensor_type == SENSOR_TYPE_SHT4X) {
		sensor_name = "SHT4X";
	} else if (g_sensor_type == SENSOR_TYPE_GXHTV4) {
		sensor_name = "GXHTV4";
	}
	
	hprintf255(request, "<h2>%s Temperature=%.1fC, Humidity=%.0f%%</h2>", sensor_name, g_temp, g_humid);
	if (channel_humid == channel_temp) {
		hprintf255(request, "WARNING: You don't have configured target channels for temp and humid results, set the first and second channel index in Pins!");
	}
}
