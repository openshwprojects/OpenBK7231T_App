

#include "../httpserver/new_http.h"

enum {
	OBK_VOLTAGE, // must match order in cmd_public.h
	OBK_CURRENT,
	OBK_POWER,
	OBK_NUM_MEASUREMENTS,
};

void DRV_Generic_Init();
void DRV_AppendInformationToHTTPIndexPage(http_request_t *request);
void DRV_OnEverySecond();
void DRV_RunQuickTick();
void DRV_StartDriver(const char *name);
void DRV_StopDriver(const char *name);
bool DRV_IsRunning(const char *name);
void DRV_OnChannelChanged(int channel,int iVal);
void SM2135_Write(byte *rgbcw);
void DRV_DGR_OnLedDimmerChange(int iVal);
void DRV_DGR_OnLedEnableAllChange(int iVal);
// OBK_POWER etc
float DRV_GetReading(int type);






