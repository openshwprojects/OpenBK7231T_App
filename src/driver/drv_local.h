
#include "../httpserver/new_http.h"

void BL0942_Init();
void BL0942_RunFrame();

void BL0937_Init();
void BL0937_RunFrame();

void CSE7766_Init();
void CSE7766_RunFrame();

void Test_Power_Init();
void Test_Power_RunFrame();

void Test_LED_Driver_Init();
void Test_LED_Driver_RunFrame();
void Test_LED_Driver_OnChannelChanged(int ch, int value);

void DRV_DGR_Init();
void DRV_DGR_RunQuickTick();
void DRV_DGR_RunEverySecond();
void DRV_DGR_Shutdown();
void DRV_DGR_OnChannelChanged(int ch, int value);

void DRV_DDP_Init();
void DRV_DDP_RunFrame();
void DRV_DDP_Shutdown();

void SM2135_Init();
void SM2135_RunFrame();
void SM2135_OnChannelChanged(int ch, int value);

void BP5758D_Init();
void BP5758D_RunFrame();
void BP5758D_OnChannelChanged(int ch, int value);

void BP1658CJ_Init();
void BP1658CJ_RunFrame();
void BP1658CJ_OnChannelChanged(int ch, int value);

void BL_Shared_Init();
void BL_ProcessUpdate(float voltage, float current, float power);
void BL09XX_AppendInformationToHTTPIndexPage(http_request_t* request);
bool DRV_IsRunning(const char* name);


void TuyaMCU_Sensor_RunFrame();
void TuyaMCU_Sensor_Init();

int BL09XX_ResetEnergyCounter(const void* context, const char* cmd, const char* args, int cmdFlags);
