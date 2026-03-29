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

#define SHT_MAX_SENSORS 4


// we'll need quite some testing for SHT3x sensor or return an error.
// keep memory low by using an general error message as const char*
static const char* ONLY_SHT3_ERROR_STR = "Only supported on SHT3x";


// Sensor type enumeration - with GXHTV4 support
typedef enum {
	SENSOR_TYPE_UNKNOWN = 0,
	SENSOR_TYPE_SHT3X = 1,
	SENSOR_TYPE_SHT4X = 2,
	SENSOR_TYPE_GXHTV4 = 3
} sensor_type_t;

const char* typeName[] =
{
	"Unknown",
	"SHT3x",
	"SHT4x",
	"GXHTV4"
};

// --------------------------------------------------------------------------
// Per-sensor state struct
// --------------------------------------------------------------------------
typedef struct {
	softI2C_t       softI2C;
	sensor_type_t   sensor_type;
	float           temp;
	float           humid;
	float           caltemp;
	float           calhum;
	bool            shtper;          // periodic mode active
	byte            channel_temp;
	byte            channel_humid;
	uint32_t        serial;          // last-read serial number (0 if unknown)
	bool            active;          // slot in use
} sht_sensor_t;

static byte usedchannels[SHT_MAX_SENSORS*2]={0};

static sht_sensor_t g_sensors[SHT_MAX_SENSORS];
static int          g_sensor_count = 0;

// Timing is shared across all sensors (one global measurement cycle)
static byte g_sht_secondsUntilNextMeasurement = 1;
static byte g_sht_secondsBetweenMeasurements  = 10;

// Convenience accessor: the "primary" sensor (index 0) used by legacy
// single-sensor commands that don't accept an index argument.
#define PRIMARY  (&g_sensors[0])


// --------------------------------------------------------------------------
// CHECK_SHT3X_SENSOR macro – now operates on a sht_sensor_t*
// --------------------------------------------------------------------------
#define CHECK_SHT3X_SENSOR_S(s, CODE) do {                      \
    if ((s)->sensor_type != SENSOR_TYPE_SHT3X) {                \
        ADDLOG_ERROR(LOG_FEATURE_SENSOR, ONLY_SHT3_ERROR_STR);  \
        return CODE;                                            \
    }                                                           \
} while (0)

// Legacy macro kept for void functions using primary sensor
#define CHECK_SHT3X_SENSOR() do {                               \
    if (PRIMARY->sensor_type != SENSOR_TYPE_SHT3X) {           \
        ADDLOG_ERROR(LOG_FEATURE_SENSOR, ONLY_SHT3_ERROR_STR); \
        return;                                                 \
    }                                                           \
} while (0)

// Legacy macro for commandResult_t functions using primary sensor
#define CHECK_SHT3X_SENSOR_CMD(CODE) \
    CHECK_SHT3X_SENSOR_S(PRIMARY, CODE)


// --------------------------------------------------------------------------
// CRC-8
// --------------------------------------------------------------------------
static uint8_t SHT_CalcCrc(uint8_t byte1, uint8_t byte2) {
	uint8_t crc = 0xFF;

	crc ^= byte1;
	for (uint8_t j = 0; j < 8; j++) {
		crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
	}
	crc ^= byte2;
	for (uint8_t j = 0; j < 8; j++) {
		crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
	}
	return crc;
}


// --------------------------------------------------------------------------
// Serial number read – now takes a sht_sensor_t* instead of global state
// --------------------------------------------------------------------------
static bool SHT_GetSerial(sht_sensor_t* s, uint8_t type) {
	uint8_t data[6];
	uint8_t crc_word1, crc_word2;
	bool ack1 = false;

	switch (type) {
		case 3:
			Soft_I2C_Start(&s->softI2C, SHT3X_I2C_ADDR);
			ack1 = Soft_I2C_WriteByte(&s->softI2C, 0x36);
			bool ack2 = false;
			if (ack1) {
				ack2 = Soft_I2C_WriteByte(&s->softI2C, 0x82);
				ADDLOG_DEBUG(LOG_FEATURE_SENSOR, "SHT3x serial: 0x36 ACK, 0x82 %s",
				             ack2 ? "ACK" : "NACK");
			} else {
				ADDLOG_DEBUG(LOG_FEATURE_SENSOR, "SHT3x serial: 0x36 NACK");
			}
			Soft_I2C_Stop(&s->softI2C);

			if (ack1 && ack2) {
				rtos_delay_milliseconds(2);
				Soft_I2C_Start(&s->softI2C, SHT3X_I2C_ADDR | 1);
				Soft_I2C_ReadBytes(&s->softI2C, data, 6);
				Soft_I2C_Stop(&s->softI2C);

				crc_word1 = SHT_CalcCrc(data[0], data[1]);
				crc_word2 = SHT_CalcCrc(data[3], data[4]);
				if (crc_word1 == data[2] && crc_word2 == data[5]) {
					s->serial = ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
					            ((uint32_t)data[3] <<  8) |  (uint32_t)data[4];
					ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT3X Serial: %08X", s->serial);
					return true;
				} else {
					ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT3X Serial CRC mismatch: %02X%02X%02X%02X",
					            data[0], data[1], data[3], data[4]);
					return false;
				}
			}
			ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT3X GetSerial command failed");
			return false;

		case 4:
			Soft_I2C_Start(&s->softI2C, SHT3X_I2C_ADDR);
			ack1 = Soft_I2C_WriteByte(&s->softI2C, 0x89);
			Soft_I2C_Stop(&s->softI2C);
			if (ack1) {
				rtos_delay_milliseconds(2);
				Soft_I2C_Start(&s->softI2C, SHT3X_I2C_ADDR | 1);
				Soft_I2C_ReadBytes(&s->softI2C, data, 6);
				Soft_I2C_Stop(&s->softI2C);

				crc_word1 = SHT_CalcCrc(data[0], data[1]);
				crc_word2 = SHT_CalcCrc(data[3], data[4]);
				if (crc_word1 == data[2] && crc_word2 == data[5]) {
					s->serial = ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
					            ((uint32_t)data[3] <<  8) |  (uint32_t)data[4];
					ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT4X Serial: %08X", s->serial);
					return true;
				} else {
					ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT4X Serial CRC mismatch: %02X%02X%02X%02X",
					            data[0], data[1], data[3], data[4]);
					return false;
				}
			}
			ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT4X GetSerial command failed");
			return false;

		default:
			ADDLOG_ERROR(LOG_FEATURE_SENSOR, "GetSerial: invalid type %i (use 3 or 4)", type);
			return false;
	}
}


// --------------------------------------------------------------------------
// Auto-detect sensor type for a given sensor slot
// --------------------------------------------------------------------------
static sensor_type_t SHT_DetectSensorType(sht_sensor_t* s) {
	ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT: Detecting sensor (SDA=%i SCL=%i)...",
	            s->softI2C.pin_data, s->softI2C.pin_clk);

	rtos_delay_milliseconds(10);

	if (SHT_GetSerial(s, 4)) {
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "Detected SHT4X");
		return SENSOR_TYPE_SHT4X;
	}
	rtos_delay_milliseconds(10);
	if (SHT_GetSerial(s, 3)) {
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "Detected SHT3X");
		return SENSOR_TYPE_SHT3X;
	}

	ADDLOG_WARN(LOG_FEATURE_SENSOR, "SHT: Detection failed, defaulting to SHT3X");
	return SENSOR_TYPE_SHT3X;
}


// --------------------------------------------------------------------------
// Low-level sensor operations (all take sht_sensor_t*)
// --------------------------------------------------------------------------
static void SHT3X_StopPer_S(sht_sensor_t* s) {
	if (s->sensor_type != SENSOR_TYPE_SHT3X) return;
	Soft_I2C_Start(&s->softI2C, SHT3X_I2C_ADDR);
	Soft_I2C_WriteByte(&s->softI2C, 0x30);
	Soft_I2C_WriteByte(&s->softI2C, 0x93);
	Soft_I2C_Stop(&s->softI2C);
	s->shtper = false;
}

static void SHT3X_StartPer_S(sht_sensor_t* s, uint8_t msb, uint8_t lsb) {
	if (s->sensor_type != SENSOR_TYPE_SHT3X) return;
	Soft_I2C_Start(&s->softI2C, SHT3X_I2C_ADDR);
	Soft_I2C_WriteByte(&s->softI2C, msb);
	Soft_I2C_WriteByte(&s->softI2C, lsb);
	Soft_I2C_Stop(&s->softI2C);
	s->shtper = true;
}

static void SHT3X_GetStatus_S(sht_sensor_t* s) {
	if (s->sensor_type != SENSOR_TYPE_SHT3X) return;
	uint8_t status[2];
	Soft_I2C_Start(&s->softI2C, SHT3X_I2C_ADDR);
	Soft_I2C_WriteByte(&s->softI2C, 0xf3);
	Soft_I2C_WriteByte(&s->softI2C, 0x2d);
	Soft_I2C_Stop(&s->softI2C);
	Soft_I2C_Start(&s->softI2C, SHT3X_I2C_ADDR | 1);
	Soft_I2C_ReadBytes(&s->softI2C, status, 2);
	Soft_I2C_Stop(&s->softI2C);
	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SHT[SDA%i]: Status: %02X %02X",
	          s->softI2C.pin_data, status[0], status[1]);
}

static void SHT3X_MeasurePercmd_S(sht_sensor_t* s) {
	if (s->sensor_type != SENSOR_TYPE_SHT3X) return;
#if WINDOWS
	s->temp  = 23.4f;
	s->humid = 56.7f;
#else
	uint8_t buff[6];
	Soft_I2C_Start(&s->softI2C, SHT3X_I2C_ADDR);
	Soft_I2C_WriteByte(&s->softI2C, 0xE0);
	Soft_I2C_WriteByte(&s->softI2C, 0x00);
	Soft_I2C_Stop(&s->softI2C);
	Soft_I2C_Start(&s->softI2C, SHT3X_I2C_ADDR | 1);
	Soft_I2C_ReadBytes(&s->softI2C, buff, 6);
	Soft_I2C_Stop(&s->softI2C);
	s->temp  = 175.0f * ((buff[0] * 256 + buff[1]) / 65535.0f) - 45.0f;
	s->humid = 100.0f * ((buff[3] * 256 + buff[4]) / 65535.0f);
#endif
	s->temp  = (int)((s->temp  + s->caltemp) * 10.0f) / 10.0f;
	s->humid = (int)( s->humid + s->calhum);

//	s->channel_temp  = g_cfg.pins.channels [s->softI2C.pin_data];
//	s->channel_humid = g_cfg.pins.channels2[s->softI2C.pin_data];
	CHANNEL_Set(s->channel_temp,  (int)(s->temp * 10), 0);
	CHANNEL_Set(s->channel_humid, (int)(s->humid),     0);
	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR,
	          "SHT[SDA%i] Period Temp:%.2fC Hum:%.0f%%",
	          s->softI2C.pin_data, s->temp, s->humid);
}

static void SHT3X_Measurecmd_S(sht_sensor_t* s) {
#if WINDOWS
	s->temp  = 23.4f;
	s->humid = 56.7f;
	const char errstr[] = "";
#else
	uint8_t buff[6];
	uint8_t crc1, crc2;

	Soft_I2C_Start(&s->softI2C, SHT3X_I2C_ADDR);
	if (s->sensor_type == SENSOR_TYPE_SHT4X || s->sensor_type == SENSOR_TYPE_GXHTV4) {
		Soft_I2C_WriteByte(&s->softI2C, 0xE0);
	} else {
		Soft_I2C_WriteByte(&s->softI2C, 0x24);
		Soft_I2C_WriteByte(&s->softI2C, 0x16);
	}
	Soft_I2C_Stop(&s->softI2C);
	rtos_delay_milliseconds(30);

	Soft_I2C_Start(&s->softI2C, SHT3X_I2C_ADDR | 1);
	Soft_I2C_ReadBytes(&s->softI2C, buff, 6);
	Soft_I2C_Stop(&s->softI2C);

	crc1 = SHT_CalcCrc(buff[0], buff[1]);
	crc2 = SHT_CalcCrc(buff[3], buff[4]);

	char errstr[128] = {0};
	if (crc1 != buff[2] || crc2 != buff[5]) {
		int off = 0;
		off += snprintf(errstr + off, sizeof(errstr) - off, "  ! CRC mismatch!");
		if (crc1 != buff[2])
			off += snprintf(errstr + off, sizeof(errstr) - off,
			                " T:0x%02X(0x%02X).", buff[2], crc1);
		if (crc2 != buff[5])
			snprintf(errstr + off, sizeof(errstr) - off,
			         " H:0x%02X(0x%02X).", buff[5], crc2);
	}

	s->temp  = 175.0f * ((buff[0] << 8 | buff[1]) / 65535.0f) - 45.0f;
	s->humid = (buff[3] << 8 | buff[4]) / 655.35f;
	if (s->sensor_type != SENSOR_TYPE_SHT3X) {
		s->humid = s->humid * 1.25f - 6.0f;
		if (s->humid > 100.0f) s->humid = 100.0f;
		if (s->humid <   0.0f) s->humid =   0.0f;
	}
#endif

	s->temp  = (int)((s->temp  + s->caltemp) * 10.0f) / 10.0f;
	s->humid = (int)( s->humid + s->calhum);

//	s->channel_temp  = g_cfg.pins.channels [s->softI2C.pin_data];
//	s->channel_humid = g_cfg.pins.channels2[s->softI2C.pin_data];
	CHANNEL_Set(s->channel_temp,  (int)(s->temp * 10), 0);
	CHANNEL_Set(s->channel_humid, (int)(s->humid),     0);

	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR,
	          "SHT[SDA%i] Temp:%.1fC Hum:%.0f%%%s",
	          s->softI2C.pin_data, s->temp, s->humid, errstr);
}


// --------------------------------------------------------------------------
// Helper: initialise one sensor slot with the given pins
// Returns pointer to the slot, or NULL if no room.
// --------------------------------------------------------------------------
static sht_sensor_t* SHT_InitSlot(int pin_sda, int pin_scl, int type_hint) {
	if (g_sensor_count >= SHT_MAX_SENSORS) {
		ADDLOG_ERROR(LOG_FEATURE_SENSOR,
		             "SHT: Max sensor count (%i) reached", SHT_MAX_SENSORS);
		return NULL;
	}
	sht_sensor_t* s = &g_sensors[g_sensor_count];
	memset(s, 0, sizeof(*s));

	s->softI2C.pin_data = pin_sda;
	s->softI2C.pin_clk  = pin_scl;
	Soft_I2C_PreInit(&s->softI2C);

	ADDLOG_INFO(LOG_FEATURE_SENSOR,
	            "SHT: Init sensor[%i] SDA=%i SCL=%i",
	            g_sensor_count, pin_sda, pin_scl);

	if (type_hint == 3) {
		s->sensor_type = SENSOR_TYPE_SHT3X;
		SHT_GetSerial(s, 3);
	} else if (type_hint == 4) {
		s->sensor_type = SENSOR_TYPE_SHT4X;
		SHT_GetSerial(s, 4);
	} else {
		s->sensor_type = SHT_DetectSensorType(s);
	}

	// Read initial status for SHT3x sensors
	SHT3X_GetStatus_S(s);

	s->active = true;
	g_sensor_count++;
	return s;
}


// --------------------------------------------------------------------------
// Command: SHT_AddSensor <sda-pin> <scl-pin> [type 3|4]
// --------------------------------------------------------------------------
commandResult_t SHT_AddSensor(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	int type_hint = 0;
	byte chan_h = 0, chan_t = 0;
	int pin_sda   = Tokenizer_GetArgInteger(0);
	int pin_scl   = Tokenizer_GetArgInteger(1);
	if (g_sensor_count >= SHT_MAX_SENSORS) {
		ADDLOG_ERROR(LOG_FEATURE_SENSOR, "SHT: Max sensor count (%i) reached", SHT_MAX_SENSORS);
		return CMD_RES_ERROR;
	}
	uint8_t argnum=Tokenizer_GetArgsCount();
	const char* arg;
	const char* found=NULL;
	for (int i=2; i<=argnum; i++) {	// first two are pins
		arg = Tokenizer_GetArg(i);
		found=NULL;
//		ADDLOG_INFO(LOG_FEATURE_DRV,"SHT3X: argument %i/%i is %s",i,argnum,arg);		

		found=strstr(arg,"chan_t=");
		if ( arg && found) {
			chan_t=atoi(found+7);
			ADDLOG_INFO(LOG_FEATURE_DRV,"SHT3X: temp channel for sensor %i is %i",g_sensor_count+1,chan_t);
		} 
		found=strstr(arg,"chan_h=");
		if ( arg && found) {
			chan_h=atoi(found+7);
			ADDLOG_INFO(LOG_FEATURE_DRV,"SHT3X: hum channel for sensor %i is %i",g_sensor_count+1,chan_h);
		} 
		found=strstr(arg,"type=");
		if ( arg && found) {
			type_hint=atoi(found+5);
			if (type_hint != 3 && type_hint != 4) {
				ADDLOG_ERROR(LOG_FEATURE_SENSOR,
					     "SHT_AddSensor: invalid type %i! Must be 3 or 4 for SHT3x/SHT4x");
				return CMD_RES_BAD_ARGUMENT;
			}
			ADDLOG_INFO(LOG_FEATURE_DRV,"SHT3X: Type for sensor %i is %i (%s)",g_sensor_count+1,type_hint,typeName[type_hint-2]);	// 0=unknown 1=sht3x 2=sht4x 3=GXHTV4 --> (t=3->1 t=4->2) --> hint - 2
		} 
	}			
	sht_sensor_t* s = SHT_InitSlot(pin_sda, pin_scl, type_hint);
	if (!s) return CMD_RES_ERROR;

	s->channel_temp = chan_t;
	s->channel_humid = chan_h;
	if (s->channel_temp != 0) {
//		g_cfg.pins.channels[pin_sda]=s->channel_temp;
		usedchannels[(g_sensor_count-1)*2]=s->channel_temp;
	}
	if (s->channel_humid != 0) {
//		g_cfg.pins.channels2[pin_sda]=s->channel_humid;
		usedchannels[(g_sensor_count-1)*2 +1]=s->channel_humid;
	}
	ADDLOG_INFO(LOG_FEATURE_SENSOR,
	            "SHT_AddSensor: Added sensor[%i] SDA=%i SCL=%i type=%s serial=%08X T-Channel=%i H-Channel=%i",
	            g_sensor_count - 1, pin_sda, pin_scl, typeName[s->sensor_type], s->serial, s->channel_temp, s->channel_humid);
	return CMD_RES_OK;
}


// --------------------------------------------------------------------------
// Command: SHT_ListSensors
// --------------------------------------------------------------------------
commandResult_t SHT_ListSensors(const void* context, const char* cmd, const char* args, int cmdFlags) {
	if (g_sensor_count == 0) {
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT_ListSensors: No sensors registered");
		return CMD_RES_OK;
	}
	ADDLOG_INFO(LOG_FEATURE_SENSOR,
	            "SHT_ListSensors: %i sensor(s) registered:", g_sensor_count);
	for (int i = 0; i < g_sensor_count; i++) {
		sht_sensor_t* s = &g_sensors[i];
		ADDLOG_INFO(LOG_FEATURE_SENSOR,
		            "  [%i] SDA=%i SCL=%i type=%-7s serial=%08X  last T=%.1fC H=%.0f%% T-Channel=%i H-Channel=",
		            i,
		            s->softI2C.pin_data,
		            s->softI2C.pin_clk,
		            typeName[s->sensor_type],
		            s->serial,
		            s->temp,
		            s->humid,
		            s->channel_temp,
		            s->channel_humid);
	}
	return CMD_RES_OK;
}


// --------------------------------------------------------------------------
// Sensor index resolver
//
// "Array +1" convention: argument 1 => sensor[0], 2 => sensor[1], etc.
// arg_index : which Tokenizer argument holds the (optional) sensor number.
// Returns pointer to the sensor, or NULL and logs an error when out of range.
// Call AFTER Tokenizer_TokenizeString().
// --------------------------------------------------------------------------
static sht_sensor_t* SHT_GetSensorByArg(const char* cmd, int arg_index, bool arg_present) {
	if (!arg_present) {
		return PRIMARY;   // default: sensor[0]
	}
	int n = Tokenizer_GetArgInteger(arg_index);
	if (n < 1 || n > g_sensor_count) {
		ADDLOG_ERROR(LOG_FEATURE_SENSOR,
		             "%s: sensor index %i out of range (1..%i)", cmd, n, g_sensor_count);
		return NULL;
	}
	return &g_sensors[n - 1];
}


// --------------------------------------------------------------------------
// Commands – primary (index 0) versions AND sensor-aware versions
// --------------------------------------------------------------------------

// --- SHT_Calibrate / SHT_CalibrateN ---
// SHT_Calibrate   <deltaT> <deltaH>          → sensor[0]
// SHT_CalibrateN  <deltaT> <deltaH> <sensor> → sensor[N-1]
static commandResult_t SHT_Calibrate_Impl(const char* cmd, const char* args, int extra_arg) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	bool has_sensor = (extra_arg && Tokenizer_GetArgsCount() >= 3);
	sht_sensor_t* s = SHT_GetSensorByArg(cmd, 2, has_sensor);
	if (!s) return CMD_RES_BAD_ARGUMENT;

	s->caltemp = Tokenizer_GetArgFloat(0);
	s->calhum  = Tokenizer_GetArgFloat(1);
	ADDLOG_INFO(LOG_FEATURE_SENSOR,
	            "SHT[SDA%i] Calibrate: temp %f  humidity %f",
	            s->softI2C.pin_data, s->caltemp, s->calhum);
	return CMD_RES_OK;
}

commandResult_t SHT3X_Calibrate(const void* context, const char* cmd, const char* args, int cmdFlags) {
	// Accepts optional 3rd argument (sensor number) for sensor selection
	return SHT_Calibrate_Impl(cmd, args, 1);
}

void SHT3X_StopPer() {
	SHT3X_StopPer_S(PRIMARY);
}

void SHT3X_StartPer(uint8_t msb, uint8_t lsb) {
	SHT3X_StartPer_S(PRIMARY, msb, lsb);
}

// SHT_LaunchPer [msb] [lsb] [sensor]
//   no args            → default bytes 0x23/0x22, sensor[0]
//   msb lsb            → given bytes,             sensor[0]
//   msb lsb <sensor>   → given bytes,             sensor[N-1]
commandResult_t SHT3X_ChangePer(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	uint8_t g_msb, g_lsb;
	int argc = Tokenizer_GetArgsCount();
	sht_sensor_t* s;
	if (argc == 0) {
		// No args: default bytes, primary sensor
		g_msb = 0x23; g_lsb = 0x22;
		s = PRIMARY;
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT ChangePer: using default 0x23 0x22");
	} else if (argc == 1 && Tokenizer_GetArgInteger(0) <= SHT_MAX_SENSORS) {
		// Single small integer: treat as sensor number, default bytes
		g_msb = 0x23; g_lsb = 0x22;
		s = SHT_GetSensorByArg(cmd, 0, true);
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT ChangePer: using default 0x23 0x22 for sensor %i",Tokenizer_GetArgInteger(0));
	} else if (argc >= 2){
		// Two or more args: first two are msb/lsb, optional third is sensor
		g_msb = Tokenizer_GetArgInteger(0);
		g_lsb = Tokenizer_GetArgInteger(1);
		s = SHT_GetSensorByArg(cmd, 2, argc >= 3);
	} else {
		ADDLOG_ERROR(LOG_FEATURE_SENSOR, "SHT ChangePer: invalid arguments");
		return CMD_RES_BAD_ARGUMENT;
	}
	if (!s) return CMD_RES_BAD_ARGUMENT;
	if (s->sensor_type != SENSOR_TYPE_SHT3X) {
		ADDLOG_ERROR(LOG_FEATURE_SENSOR, ONLY_SHT3_ERROR_STR);
		return CMD_RES_ERROR;
	}
	SHT3X_StopPer_S(s);
	rtos_delay_milliseconds(25);
	SHT3X_StartPer_S(s, g_msb, g_lsb);
	ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT[SDA%i] ChangePer done", s->softI2C.pin_data);
	return CMD_RES_OK;
}

// SHT_Heater <0|1> [sensor]  — no sensor arg = sensor[0]
commandResult_t SHT3X_Heater(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	int state = Tokenizer_GetArgInteger(0);
	sht_sensor_t* s = SHT_GetSensorByArg(cmd, 1, Tokenizer_GetArgsCount() >= 2);
	if (!s) return CMD_RES_BAD_ARGUMENT;
	if (s->sensor_type != SENSOR_TYPE_SHT3X) {
		ADDLOG_ERROR(LOG_FEATURE_SENSOR, ONLY_SHT3_ERROR_STR);
		return CMD_RES_ERROR;
	}
	Soft_I2C_Start(&s->softI2C, SHT3X_I2C_ADDR);
	Soft_I2C_WriteByte(&s->softI2C, 0x30);
	Soft_I2C_WriteByte(&s->softI2C, state ? 0x6D : 0x66);
	Soft_I2C_Stop(&s->softI2C);
	ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT[SDA%i] Heater %s",
	            s->softI2C.pin_data, state ? "activated" : "deactivated");
	return CMD_RES_OK;
}

void SHT3X_MeasurePercmd() {
	SHT3X_MeasurePercmd_S(PRIMARY);
}

// SHT_MeasurePer [sensor]  — no arg = sensor[0], arg = sensor[N-1]
commandResult_t SHT3X_MeasurePer(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	sht_sensor_t* s = SHT_GetSensorByArg(cmd, 0, Tokenizer_GetArgsCount() >= 1);
	if (!s) return CMD_RES_BAD_ARGUMENT;
	if (s->sensor_type != SENSOR_TYPE_SHT3X) {
		ADDLOG_ERROR(LOG_FEATURE_SENSOR, ONLY_SHT3_ERROR_STR);
		return CMD_RES_ERROR;
	}
	SHT3X_MeasurePercmd_S(s);
	return CMD_RES_OK;
}

void SHT3X_Measurecmd() {
	SHT3X_Measurecmd_S(PRIMARY);
}

// SHT_Measure [sensor]
//   no argument  → measure ALL sensors
//   argument N   → measure sensor[N-1] only
commandResult_t SHT3X_Measure(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if (Tokenizer_GetArgsCount() == 0) {
		// No argument: measure all sensors
		for (int i = 0; i < g_sensor_count; i++) {
			if (g_sensors[i].active) {
				SHT3X_Measurecmd_S(&g_sensors[i]);
			}
		}
	} else {
		sht_sensor_t* s = SHT_GetSensorByArg(cmd, 0, true);
		if (!s) return CMD_RES_BAD_ARGUMENT;
		SHT3X_Measurecmd_S(s);
	}
	return CMD_RES_OK;
}

void SHT3X_StopDriver() {
	for (int i = 0; i < g_sensor_count; i++) {
		sht_sensor_t* s = &g_sensors[i];
		if (s->sensor_type == SENSOR_TYPE_SHT4X || s->sensor_type == SENSOR_TYPE_GXHTV4) {
			SHT3X_StopPer_S(s);
			Soft_I2C_Start(&s->softI2C, SHT3X_I2C_ADDR);
			Soft_I2C_WriteByte(&s->softI2C, 0x09);
			Soft_I2C_Stop(&s->softI2C);
		} else {
			SHT3X_StopPer_S(s);
			Soft_I2C_Start(&s->softI2C, SHT3X_I2C_ADDR);
			Soft_I2C_WriteByte(&s->softI2C, 0x30);
			Soft_I2C_WriteByte(&s->softI2C, 0xA2);
			Soft_I2C_Stop(&s->softI2C);
		}
	}
}

// SHT_StopPer [sensor]  — no arg = sensor[0]
commandResult_t SHT3X_StopPerCmd(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	sht_sensor_t* s = SHT_GetSensorByArg(cmd, 0, Tokenizer_GetArgsCount() >= 1);
	if (!s) return CMD_RES_BAD_ARGUMENT;
	if (s->sensor_type != SENSOR_TYPE_SHT3X) {
		ADDLOG_ERROR(LOG_FEATURE_SENSOR, ONLY_SHT3_ERROR_STR);
		return CMD_RES_ERROR;
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SHT[SDA%i]: Stopping periodic capture", s->softI2C.pin_data);
	SHT3X_StopPer_S(s);
	return CMD_RES_OK;
}

/*
void SHT3X_GetStatus() {
	SHT3X_GetStatus_S(PRIMARY);
}
*/

// SHT_GetStatus [sensor]  — no arg = sensor[0]
commandResult_t SHT3X_GetStatusCmd(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	sht_sensor_t* s = SHT_GetSensorByArg(cmd, 0, Tokenizer_GetArgsCount() >= 1);
	if (!s) return CMD_RES_BAD_ARGUMENT;
	if (s->sensor_type != SENSOR_TYPE_SHT3X) {
		ADDLOG_ERROR(LOG_FEATURE_SENSOR, ONLY_SHT3_ERROR_STR);
		return CMD_RES_ERROR;
	}
	SHT3X_GetStatus_S(s);
	return CMD_RES_OK;
}

/*
void SHT3X_ClearStatus() {
	if (PRIMARY->sensor_type != SENSOR_TYPE_SHT3X) return;
	Soft_I2C_Start(&PRIMARY->softI2C, SHT3X_I2C_ADDR);
	Soft_I2C_WriteByte(&PRIMARY->softI2C, 0x30);
	Soft_I2C_WriteByte(&PRIMARY->softI2C, 0x41);
	Soft_I2C_Stop(&PRIMARY->softI2C);
	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SHT: Clear status");
}
*/

// SHT_ClearStatus [sensor]  — no arg = sensor[0]
commandResult_t SHT3X_ClearStatusCmd(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	sht_sensor_t* s = SHT_GetSensorByArg(cmd, 0, Tokenizer_GetArgsCount() >= 1);
	if (!s) return CMD_RES_BAD_ARGUMENT;
	if (s->sensor_type != SENSOR_TYPE_SHT3X) {
		ADDLOG_ERROR(LOG_FEATURE_SENSOR, ONLY_SHT3_ERROR_STR);
		return CMD_RES_ERROR;
	}
	Soft_I2C_Start(&s->softI2C, SHT3X_I2C_ADDR);
	Soft_I2C_WriteByte(&s->softI2C, 0x30);
	Soft_I2C_WriteByte(&s->softI2C, 0x41);
	Soft_I2C_Stop(&s->softI2C);
	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SHT[SDA%i]: Clear status", s->softI2C.pin_data);
	return CMD_RES_OK;
}

bool SHT3X_IsUsedChannel(int c){
	for (int i=2; c>0 && i<SHT_MAX_SENSORS*2; i++){
		if (usedchannels[i] == c) return true;
	}
	return false;
}

// --------------------------------------------------------------------------
// Alert helpers (primary sensor / SHT3x only)
// --------------------------------------------------------------------------
static void SHT3X_ReadAlertLimitData(float* humidity, float* temperature) {
	if (PRIMARY->sensor_type != SENSOR_TYPE_SHT3X) return;
	uint8_t data[2];
	Soft_I2C_Start(&PRIMARY->softI2C, SHT3X_I2C_ADDR | 1);
	Soft_I2C_ReadBytes(&PRIMARY->softI2C, data, 2);
	Soft_I2C_Stop(&PRIMARY->softI2C);
	uint16_t finaldata = (data[0] << 8) | data[1];
	(*humidity) = 100.0f * (finaldata & 0xFE00) / 65535.0f;
	finaldata <<= 7;
	(*temperature) = (175.0f * finaldata / 65535.0f) - 45.0f;
}

void SHT3X_WriteAlertLimitData(float humidity, float temperature) {
	if (PRIMARY->sensor_type != SENSOR_TYPE_SHT3X) return;
	if (humidity < 0.0f || humidity > 100.0f || temperature < -45.0f || temperature > 130.0f) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "SHT SetAlert: value out of limit");
		return;
	}
	uint16_t rawH = humidity    / 100.0f * 65535.0f;
	uint16_t rawT = (temperature + 45.0f) / 175.0f * 65535.0f;
	uint16_t finaldata = (rawH & 0xFE00) | ((rawT >> 7) & 0x01FF);
	uint8_t data[2] = { finaldata >> 8, finaldata & 0xFF };
	uint8_t checksum = SHT_CalcCrc(data[0], data[1]);
	Soft_I2C_WriteByte(&PRIMARY->softI2C, data[0]);
	Soft_I2C_WriteByte(&PRIMARY->softI2C, data[1]);
	Soft_I2C_WriteByte(&PRIMARY->softI2C, checksum);
}

void SHT3X_GetAlertLimits() {
	if (PRIMARY->sensor_type != SENSOR_TYPE_SHT3X) return;
	float tLS, tLC, tHC, tHS, hLS, hLC, hHC, hHS;

	Soft_I2C_Start(&PRIMARY->softI2C, SHT3X_I2C_ADDR);
	Soft_I2C_WriteByte(&PRIMARY->softI2C, 0xe1); Soft_I2C_WriteByte(&PRIMARY->softI2C, 0x1f);
	Soft_I2C_Stop(&PRIMARY->softI2C);
	SHT3X_ReadAlertLimitData(&hHS, &tHS);

	Soft_I2C_Start(&PRIMARY->softI2C, SHT3X_I2C_ADDR);
	Soft_I2C_WriteByte(&PRIMARY->softI2C, 0xe1); Soft_I2C_WriteByte(&PRIMARY->softI2C, 0x14);
	Soft_I2C_Stop(&PRIMARY->softI2C);
	SHT3X_ReadAlertLimitData(&hHC, &tHC);

	Soft_I2C_Start(&PRIMARY->softI2C, SHT3X_I2C_ADDR);
	Soft_I2C_WriteByte(&PRIMARY->softI2C, 0xe1); Soft_I2C_WriteByte(&PRIMARY->softI2C, 0x09);
	Soft_I2C_Stop(&PRIMARY->softI2C);
	SHT3X_ReadAlertLimitData(&hLC, &tLC);

	Soft_I2C_Start(&PRIMARY->softI2C, SHT3X_I2C_ADDR);
	Soft_I2C_WriteByte(&PRIMARY->softI2C, 0xe1); Soft_I2C_WriteByte(&PRIMARY->softI2C, 0x02);
	Soft_I2C_Stop(&PRIMARY->softI2C);
	SHT3X_ReadAlertLimitData(&hLS, &tLS);

	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR,
	          "SHT Alert Temp: %f / %f / %f / %f", tLS, tLC, tHC, tHS);
	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR,
	          "SHT Alert Hum:  %f / %f / %f / %f", hLS, hLC, hHC, hHS);
}

commandResult_t SHT3X_ReadAlertCmd(const void* context, const char* cmd, const char* args, int cmdFlags) {
	CHECK_SHT3X_SENSOR_CMD(CMD_RES_ERROR);
	SHT3X_GetAlertLimits();
	return CMD_RES_OK;
}

commandResult_t SHT3X_SetAlertCmd(const void* context, const char* cmd, const char* args, int cmdFlags) {
	CHECK_SHT3X_SENSOR_CMD(CMD_RES_ERROR);
	float tHS, tLS, tLC, tHC, hHS, hLS, hLC, hHC;
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 4)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	tHS = Tokenizer_GetArgFloat(0);
	tLS = Tokenizer_GetArgFloat(1);
	tLC = tLS + 0.5f;
	tHC = tHS - 0.5f;
	hHS = Tokenizer_GetArgFloat(2);
	hLS = Tokenizer_GetArgFloat(3);
	hLC = hLS + 2.0f;
	hHC = hHS - 2.0f;

	Soft_I2C_Start(&PRIMARY->softI2C, SHT3X_I2C_ADDR);
	Soft_I2C_WriteByte(&PRIMARY->softI2C, 0x61); Soft_I2C_WriteByte(&PRIMARY->softI2C, 0x1d);
	SHT3X_WriteAlertLimitData(hHS, tHS);
	Soft_I2C_Stop(&PRIMARY->softI2C);

	Soft_I2C_Start(&PRIMARY->softI2C, SHT3X_I2C_ADDR);
	Soft_I2C_WriteByte(&PRIMARY->softI2C, 0x61); Soft_I2C_WriteByte(&PRIMARY->softI2C, 0x16);
	SHT3X_WriteAlertLimitData(hHC, tHC);
	Soft_I2C_Stop(&PRIMARY->softI2C);

	Soft_I2C_Start(&PRIMARY->softI2C, SHT3X_I2C_ADDR);
	Soft_I2C_WriteByte(&PRIMARY->softI2C, 0x61); Soft_I2C_WriteByte(&PRIMARY->softI2C, 0x0B);
	SHT3X_WriteAlertLimitData(hLC, tLC);
	Soft_I2C_Stop(&PRIMARY->softI2C);

	Soft_I2C_Start(&PRIMARY->softI2C, SHT3X_I2C_ADDR);
	Soft_I2C_WriteByte(&PRIMARY->softI2C, 0x61); Soft_I2C_WriteByte(&PRIMARY->softI2C, 0x00);
	SHT3X_WriteAlertLimitData(hLS, tLS);
	Soft_I2C_Stop(&PRIMARY->softI2C);

	ADDLOG_INFO(LOG_FEATURE_SENSOR,
	            "SHT SetAlert temp %f/%f  hum %f/%f", tLS, tHS, hLS, hHS);
	return CMD_RES_OK;
}

commandResult_t SHT_cycle(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_sht_secondsBetweenMeasurements = Tokenizer_GetArgInteger(0);
	ADDLOG_INFO(LOG_FEATURE_CMD, "SHT: measurement every %i seconds",
	            g_sht_secondsBetweenMeasurements);
	return CMD_RES_OK;
}

commandResult_t SHT_SetSensorType(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	int type = Tokenizer_GetArgInteger(0);
	sht_sensor_t* s = SHT_GetSensorByArg(cmd, 1, Tokenizer_GetArgsCount() >= 2);
	if (!s) {
		ADDLOG_ERROR(LOG_FEATURE_SENSOR, "Invalid sensor (%i) selected",Tokenizer_GetArgIntegerDefault(1, 1));
		return CMD_RES_BAD_ARGUMENT;
	}
	if (type == 3)      { s->sensor_type = SENSOR_TYPE_SHT3X;}
	else if (type == 4) { s->sensor_type = SENSOR_TYPE_SHT4X;}
	else if (type == 5) { s->sensor_type = SENSOR_TYPE_GXHTV4;}
	else {
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "Invalid type. Use 3=SHT3X, 4=SHT4X, 5=GXHTV4");
		return CMD_RES_BAD_ARGUMENT;
	}
	ADDLOG_INFO(LOG_FEATURE_SENSOR, "Type set to %s",typeName[s->sensor_type]);
	return CMD_RES_OK;
}


// --------------------------------------------------------------------------
// Init
// --------------------------------------------------------------------------
void SHT3X_Init() {
	memset(g_sensors, 0, sizeof(g_sensors));
	g_sensor_count = 0;

	// Set up the primary (index 0) sensor using configured pins
	int pin_scl = PIN_FindPinIndexForRole(IOR_SHT3X_CLK, 9);
	int pin_sda = PIN_FindPinIndexForRole(IOR_SHT3X_DAT, 14);
	sht_sensor_t* s = SHT_InitSlot(pin_sda, pin_scl, 0 /* auto-detect */);
	if (s) {
		s->channel_temp  = g_cfg.pins.channels[s->softI2C.pin_data];
		s->channel_humid = g_cfg.pins.channels2[s->softI2C.pin_data];
	}
	//cmddetail:{"name":"SHT_cycle","args":"[int]",
	//cmddetail:"descr":"Interval between measurements in seconds (default 10, max 255).",
	//cmddetail:"fn":"SHT_cycle","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_cycle 60"}
	CMD_RegisterCommand("SHT_cycle",       SHT_cycle,          NULL);

	//cmddetail:{"name":"SHT_SetType","args":"[type] [sensor (optional)]",
	//cmddetail:"descr":"Set primary sensor type: 3=SHT3X, 4=SHT4X, 5=GXHTV4",
	//cmddetail:"fn":"SHT_SetSensorType","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_SetType 4 [sensor # - default sensor 1]"}
	CMD_RegisterCommand("SHT_SetType",     SHT_SetSensorType,  NULL);

	//cmddetail:{"name":"SHT_AddSensor","args":"[sda-pin] [scl-pin] [chan_t=<temperature channel> (optional, 0 if ommited)] [chan_h=<humidity channel> (optional, 0 if ommited)] [type=3or4 (optional, autodetect if ommited)]",
	//cmddetail:"descr":"Add an additional SHT sensor on the given pins. Type is auto-detected if omitted.",
	//cmddetail:"fn":"SHT_AddSensor","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_AddSensor 4 5\nSHT_AddSensor 4 5 type=3  chan_t=4 chan_h=5"}
	CMD_RegisterCommand("SHT_AddSensor",   SHT_AddSensor,      NULL);

	//cmddetail:{"name":"SHT_ListSensors","args":"",
	//cmddetail:"descr":"List all registered SHT sensors with pins, type and serial number.",
	//cmddetail:"fn":"SHT_ListSensors","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_ListSensors"}
	CMD_RegisterCommand("SHT_ListSensors", SHT_ListSensors,    NULL);

	//cmddetail:{"name":"SHT_Calibrate","args":"[DeltaTemp][DeltaHumidity] [sensor (optional)]",
	//cmddetail:"descr":"Calibrate an SHT sensor. Optional 3rd arg selects sensor (1=first, 2=second, ...). Defaults to sensor 1.",
	//cmddetail:"fn":"SHT3X_Calibrate","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_Calibrate -4 10\nSHT_Calibrate -2 5 2"}
	CMD_RegisterCommand("SHT_Calibrate",   SHT3X_Calibrate,    NULL);
	//cmddetail:{"name":"SHT_MeasurePer","args":"[sensor (optional)]",
	//cmddetail:"descr":"Retrieve periodic measurement. Optional arg selects sensor (1=first, ...). Defaults to sensor 1.",
	//cmddetail:"fn":"SHT3X_MeasurePer","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_MeasurePer\nSHT_MeasurePer 2"}
	CMD_RegisterCommand("SHT_MeasurePer",  SHT3X_MeasurePer,   NULL);
	//cmddetail:{"name":"SHT_LaunchPer","args":"[msb][lsb] [sensor (optional)]",
	//cmddetail:"descr":"Launch/change periodic capture. Optional 3rd arg selects sensor (1=first, ...). Defaults to sensor 1.",
	//cmddetail:"fn":"SHT3X_ChangePer","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_LaunchPer 0x23 0x22\nSHT_LaunchPer 0x23 0x22 2"}
	CMD_RegisterCommand("SHT_LaunchPer",   SHT3X_ChangePer,    NULL);
	//cmddetail:{"name":"SHT_StopPer","args":"[sensor (optional)]",
	//cmddetail:"descr":"Stop periodic capture. Optional arg selects sensor (1=first, ...). Defaults to sensor 1.",
	//cmddetail:"fn":"SHT3X_StopPerCmd","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_StopPer\nSHT_StopPer 2"}
	CMD_RegisterCommand("SHT_StopPer",     SHT3X_StopPerCmd,   NULL);
	//cmddetail:{"name":"SHT_Measure","args":"[sensor (optional)]",
	//cmddetail:"descr":"One-shot measurement. No arg = ALL sensors. Optional arg selects one sensor (1=first, ...).",
	//cmddetail:"fn":"SHT3X_Measure","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_Measure\nSHT_Measure 2"}
	CMD_RegisterCommand("SHT_Measure",     SHT3X_Measure,      NULL);
	//cmddetail:{"name":"SHT_Heater","args":"[1or0] [sensor (optional)]",
	//cmddetail:"descr":"Activate/deactivate heater (SHT3x only). Optional 2nd arg selects sensor (1=first, ...). Defaults to sensor 1.",
	//cmddetail:"fn":"SHT3X_Heater","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_Heater 1\nSHT_Heater 0 2"}
	CMD_RegisterCommand("SHT_Heater",      SHT3X_Heater,       NULL);
	//cmddetail:{"name":"SHT_GetStatus","args":"[sensor (optional)]",
	//cmddetail:"descr":"Get sensor status (SHT3x only). Optional arg selects sensor (1=first, ...). Defaults to sensor 1.",
	//cmddetail:"fn":"SHT3X_GetStatusCmd","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_GetStatus\nSHT_GetStatus 2"}
	CMD_RegisterCommand("SHT_GetStatus",   SHT3X_GetStatusCmd, NULL);
	//cmddetail:{"name":"SHT_ClearStatus","args":"[sensor (optional)]",
	//cmddetail:"descr":"Clear sensor status (SHT3x only). Optional arg selects sensor (1=first, ...). Defaults to sensor 1.",
	//cmddetail:"fn":"SHT3X_ClearStatusCmd","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_ClearStatus\nSHT_ClearStatus 2"}
	CMD_RegisterCommand("SHT_ClearStatus", SHT3X_ClearStatusCmd, NULL);

	// this two will only work on first sensor (and are only valid for SHT3x!!!
	
	//cmddetail:{"name":"SHT_ReadAlert","args":"",
	//cmddetail:"descr":"Get Sensor alert configuration - only for first sensor and if it's an SHT3x sensor",
	//cmddetail:"fn":"SHT3X_ReadAlertCmd","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_ReadAlertCmd"}
	CMD_RegisterCommand("SHT_ReadAlert", SHT3X_ReadAlertCmd, NULL);
	//cmddetail:{"name":"SHT_SetAlert","args":"[temp_high, temp_low, hum_high, hum_low]",
	//cmddetail:"descr":"Set Sensor alert configuration - only for first sensor and if it's an SHT3x sensor",
	//cmddetail:"fn":"SHT3X_SetAlertCmd","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_SetAlertCmd"}
	CMD_RegisterCommand("SHT_SetAlert", SHT3X_SetAlertCmd, NULL);
}


// --------------------------------------------------------------------------
// Periodic tick – measure all active sensors
// --------------------------------------------------------------------------
void SHT3X_OnEverySecond() {
	if (g_sht_secondsUntilNextMeasurement <= 0) {
		for (int i = 0; i < g_sensor_count; i++) {
			sht_sensor_t* s = &g_sensors[i];
			if (!s->active) continue;
			if (s->shtper) {
				SHT3X_MeasurePercmd_S(s);
			} else {
				SHT3X_Measurecmd_S(s);
			}
		}
		g_sht_secondsUntilNextMeasurement = g_sht_secondsBetweenMeasurements;
	}
	if (g_sht_secondsUntilNextMeasurement > 0) {
		g_sht_secondsUntilNextMeasurement--;
	}
}


// --------------------------------------------------------------------------
// HTTP status page
// --------------------------------------------------------------------------
void SHT3X_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState) {
	if (bPreState) return;
	for (int i = 0; i < g_sensor_count; i++) {
		sht_sensor_t* s = &g_sensors[i];
		const char* name = "SHT3X";
		if (s->sensor_type == SENSOR_TYPE_SHT4X)  name = "SHT4X";
		if (s->sensor_type == SENSOR_TYPE_GXHTV4) name = "GXHTV4";
		hprintf255(request,
		           "<h2>%s[%i] (SDA%i/SCL%i) T=%.1f&deg;C H=%.0f%%</h2>",
		           name, i, s->softI2C.pin_data, s->softI2C.pin_clk,
		           s->temp, s->humid);
		if (s->channel_humid == s->channel_temp) {
			hprintf255(request,
			           "WARNING: sensor[%i] has no distinct channels configured!", i);
		}
	}
}
