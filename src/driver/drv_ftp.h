#ifndef __DRV_FTP_H__
#define __DRV_FTP_H__

#include "../httpserver/new_http.h"

void DRV_FTP_Init(void);
void DRV_FTP_RunEverySecond(void);
void DRV_FTP_RunQuickTick(void);
void DRV_FTP_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState);
void DRV_FTP_Shutdown(void);

#endif /* __DRV_FTP_H__ */
