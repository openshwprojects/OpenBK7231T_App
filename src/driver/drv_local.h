
#include "../httpserver/new_http.h"

void BL0942_Init();
void BL0942_RunFrame();

void BL0937_Init();
void BL0937_RunFrame();

void CSE7766_Init();
void CSE7766_RunFrame();

void DRV_DGR_Init();
void DRV_DGR_RunFrame();
void DRV_DGR_Shutdown();
void DRV_DGR_OnChannelChanged(int ch, int value);

void SM2135_Init();
void SM2135_RunFrame();
void SM2135_OnChannelChanged(int ch, int value);

void BL_ProcessUpdate(float voltage, float current, float power);
void BL09XX_AppendInformationToHTTPIndexPage(http_request_t *request);
bool DRV_IsRunning(const char *name);




