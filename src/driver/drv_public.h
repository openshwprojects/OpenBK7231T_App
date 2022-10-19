

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
    OBK_NUM_EMUNS_MAX
};

#define OBK_NUM_COUNTERS            (OBK_NUM_EMUNS_MAX-OBK_NUM_MEASUREMENTS)
#define OBK_NUM_SENSOR_COUNT         OBK_NUM_EMUNS_MAX

// MQTT names of sensors (voltage, current, power)
extern const char *sensor_mqttNames[];
extern const char *counter_mqttNames[];
extern const char *counter_devClasses[];

void DRV_Generic_Init();
void DRV_AppendInformationToHTTPIndexPage(http_request_t *request);
void DRV_OnEverySecond();
void DRV_RunQuickTick();
void DRV_StartDriver(const char *name);
void DRV_StopDriver(const char *name);
bool DRV_IsRunning(const char *name);
void DRV_OnChannelChanged(int channel,int iVal);
void SM2135_Write(byte *rgbcw);
void BP5758D_Write(byte *rgbcw);
void BP1658CJ_Write(byte *rgbcw);
void DRV_DGR_OnLedDimmerChange(int iVal);
void DRV_DGR_OnLedEnableAllChange(int iVal);

// OBK_POWER etc
float DRV_GetReading(int type);
bool DRV_IsMeasuringPower();
