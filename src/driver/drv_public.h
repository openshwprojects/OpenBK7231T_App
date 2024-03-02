#ifndef __DRV_PUBLIC_H__
#define __DRV_PUBLIC_H__

#include "../httpserver/new_http.h"

typedef enum energySensor_e {
	OBK__FIRST = 0,
	OBK_VOLTAGE = OBK__FIRST, // must match order in cmd_public.h
	OBK_CURRENT,
	OBK_POWER,
	OBK_POWER_APPARENT,
	OBK_POWER_REACTIVE,
	OBK_POWER_FACTOR,
	OBK_CONSUMPTION_TOTAL,
	OBK__NUM_MEASUREMENTS = OBK_CONSUMPTION_TOTAL,

	// TODO OBK_CONSUMPTION_LAST_HOUR is actally "sum of consumption stats recording period"
	// and won't correspond to 'last hour' unless cmd SetupEnergyStats is enabled and configured to record one hour
	// e.g. 'SetupEnergyStats 1 60 60 0': 60 sec intervals, 60 samples
	OBK_CONSUMPTION_LAST_HOUR,	
	//OBK_CONSUMPTION_STATS, // represents a variable size array of energy samples, not a sensor
	// below here are sensors that are assumed to require NTP driver
	OBK_CONSUMPTION__DAILY_FIRST, //daily consumptions are assumed to be in chronological order
	OBK_CONSUMPTION_TODAY = OBK_CONSUMPTION__DAILY_FIRST, 
	OBK_CONSUMPTION_YESTERDAY,
	OBK_CONSUMPTION_2_DAYS_AGO,
	OBK_CONSUMPTION_3_DAYS_AGO,
	OBK_CONSUMPTION__DAILY_LAST = OBK_CONSUMPTION_3_DAYS_AGO,

	OBK_CONSUMPTION_CLEAR_DATE,
	OBK__LAST = OBK_CONSUMPTION_CLEAR_DATE,
	OBK__NUM_SENSORS,
} energySensor_t;

typedef struct energySensorNames_s {	
	const char* const hass_dev_class;
	const char* const units;
	const char* const name_friendly;
	const char* const name_mqtt;
	const char* const hass_uniq_id_suffix; //keep identifiers persistent in case OBK_ENERG_SENSOR changes
} energySensorNames_t;

extern int g_dhtsCount;

void DRV_Generic_Init();
void DRV_AppendInformationToHTTPIndexPage(http_request_t* request);
void DRV_OnEverySecond();
void DHT_OnEverySecond();
void DHT_OnPinsConfigChanged();
void DRV_RunQuickTick();
void DRV_StartDriver(const char* name);
void DRV_StopDriver(const char* name);
// right now only used by simulator
void DRV_ShutdownAllDrivers();
bool DRV_IsRunning(const char* name);
void DRV_OnChannelChanged(int channel, int iVal);
#if PLATFORM_BK7231N
void SM16703P_setMultiplePixel(uint32_t pixel, uint8_t *data, bool push);
#endif
void SM2135_Write(float* rgbcw);
void BP5758D_Write(float* rgbcw);
void BP1658CJ_Write(float* rgbcw);
void SM2235_Write(float* rgbcw);
void KP18058_Write(float *rgbcw);
void DRV_DGR_OnLedDimmerChange(int iVal);
void DRV_DGR_OnLedEnableAllChange(int iVal);
void DRV_DGR_OnLedFinalColorsChange(byte rgbcw[5]);

// OBK_POWER etc
float DRV_GetReading(energySensor_t type);
energySensorNames_t* DRV_GetEnergySensorNames(energySensor_t type);
bool DRV_IsMeasuringPower();
bool DRV_IsMeasuringBattery();
bool DRV_IsSensor();
void BL09XX_SaveEmeteringStatistics();

// TuyaMCU exports for LED
void TuyaMCU_OnRGBCWChange(const float *rgbcw, int bLightEnableAll, int iLightMode, float brightnessRange01, float temperatureRange01);
bool TuyaMCU_IsLEDRunning();


#endif /* __DRV_PUBLIC_H__ */

