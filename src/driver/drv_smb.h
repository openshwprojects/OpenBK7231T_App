#ifndef __DRV_SMB_H__
#define __DRV_SMB_H__

#include "../httpserver/new_http.h"

void DRV_SMB_Init(void);
void DRV_SMB_Shutdown(void);
void DRV_SMB_RunEverySecond(void);
void DRV_SMB_RunQuickTick(void);
void DRV_SMB_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState);

#endif /* __DRV_SMB_H__ */
