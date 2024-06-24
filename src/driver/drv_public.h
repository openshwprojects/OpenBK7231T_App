#ifndef __DRV_PUBLIC_H__
#define __DRV_PUBLIC_H__

#include "../httpserver/new_http.h"
#include "../new_common.h"
#include <stdint.h>

//These are indexes, acceptable by DRV_GetReading() call
typedef enum reading_e {
	OBK__FIRST = 0,
	OBK_VOLTAGE = OBK__FIRST, // must match order in cmd_public.h
	OBK_CURRENT,
	OBK_POWER,
	OBK_POWER_APPARENT,
	OBK_POWER_REACTIVE,
	OBK_POWER_FACTOR,
	OBK_CONSUMPTION_TOTAL,
	OBK_CONSUMPTION_LAST_HOUR,
	OBK_CONSUMPTION__DAILY_FIRST, //daily consumptions are assumed to be in chronological order
	OBK_CONSUMPTION_TODAY = OBK_CONSUMPTION__DAILY_FIRST,
	OBK_CONSUMPTION_YESTERDAY,
	OBK_CONSUMPTION_2_DAYS_AGO,
	OBK_CONSUMPTION_3_DAYS_AGO,
	OBK_CONSUMPTION__DAILY_LAST = OBK_CONSUMPTION_3_DAYS_AGO,
	OBK_CONSUMPTION_CLEAR_DATE,
	OBK__LAST = OBK_CONSUMPTION_CLEAR_DATE,
	OBK__NUM_READINGS, //must be the last one
} reading_t;

extern double readings[OBK__NUM_READINGS];

//Used in hass_init_energy_sensor_device_info() so declared as public type.
typedef struct energySensorNames_s {
	const char* const hass_dev_class;
	const char* const units;
	const char* const name_friendly;
	const char* const name_mqtt;
	const uint32_t hass_uniq_index; //keep identifiers persistent in case OBK_ENERG_SENSOR changes
} energySensorNames_t;

extern int g_dhtsCount;

void DRV_Generic_Init();
void DRV_AppendInformationToHTTPIndexPage(http_request_t* request);
void DRV_OnEverySecond();
void DHT_OnEverySecond();
void DHT_OnPinsConfigChanged();
bool DRV_PublishHASSDevices(const char* topic);
void DRV_RunQuickTick();
void DRV_StartDriver(const char* name);
void DRV_StopDriver(const char* name);
// right now only used by simulator
void DRV_ShutdownAllDrivers();
void DRV_SaveState();
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
float DRV_GetReading(reading_t type);
bool DRV_IsMeasuringPower();
bool DRV_IsMeasuringBattery();
bool DRV_IsSensor();

// TuyaMCU exports for LED
void TuyaMCU_OnRGBCWChange(const float *rgbcw, int bLightEnableAll, int iLightMode, float brightnessRange01, float temperatureRange01);
bool TuyaMCU_IsLEDRunning();


#endif /* __DRV_PUBLIC_H__ */

