#ifndef __DRV_HP2620_PANEL_H__
#define __DRV_HP2620_PANEL_H__

#include "../new_common.h"
#include "../httpserver/new_http.h"

void HP2620Panel_Init(void);
void HP2620Panel_OnEverySecond(void);
void HP2620Panel_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState);
void HP2620Panel_RunQuickTick(void);
void HP2620Panel_Stop(void);
void HP2620Panel_OnChannelChanged(int ch, int val);

#endif /* __DRV_HP2620_PANEL_H__ */
