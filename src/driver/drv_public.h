#ifndef __DRV_PUBLIC_H__
#define __DRV_PUBLIC_H__

#include "../httpserver/new_http.h"

enum {
	OBK_VOLTAGE, // must match order in cmd_public.h
	OBK_CURRENT,
	OBK_POWER,
	OBK_NUM_MEASUREMENTS,
};

enum {
	OBK_CONSUMPTION_TOTAL = OBK_NUM_MEASUREMENTS,
	OBK_CONSUMPTION_LAST_HOUR,
	OBK_CONSUMPTION_STATS,
	OBK_CONSUMPTION_YESTERDAY,
	OBK_CONSUMPTION_TODAY,
	OBK_CONSUMPTION_CLEAR_DATE,
	OBK_NUM_EMUNS_MAX
};

#define OBK_NUM_COUNTERS            (OBK_NUM_EMUNS_MAX-OBK_NUM_MEASUREMENTS)
#define OBK_NUM_SENSOR_COUNT         OBK_NUM_EMUNS_MAX

// MQTT names of sensors (voltage, current, power)
extern const char* sensor_mqttNames[];
extern const char* sensor_mqtt_device_classes[];
extern const char* sensor_mqtt_device_units[];
extern const char* counter_mqttNames[];
extern const char* counter_devClasses[];
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
void SM2135_Write(float* rgbcw);
void BP5758D_Write(float* rgbcw);
void BP1658CJ_Write(float* rgbcw);
void SM2235_Write(float* rgbcw);
void DRV_DGR_OnLedDimmerChange(int iVal);
void DRV_DGR_OnLedEnableAllChange(int iVal);
void DRV_DGR_OnLedFinalColorsChange(byte rgbcw[5]);

// OBK_POWER etc
float DRV_GetReading(int type);
bool DRV_IsMeasuringPower();
bool DRV_IsMeasuringBattery();
bool DRV_IsSensor();
void BL09XX_SaveEmeteringStatistics();

#endif /* __DRV_PUBLIC_H__ */

