
#include "../httpserver/new_http.h"

void BL0942_Init();
void BL0942_RunFrame();
void BL0942_AppendInformationToHTTPIndexPage(http_request_t *request);

void BL0937_Init();
void BL0937_RunFrame();
void BL0937_AppendInformationToHTTPIndexPage(http_request_t *request);

void DRV_DGR_Init();
void DRV_DGR_RunFrame();

void BL_ProcessUpdate(float voltage, float current, float power);

