#ifndef __DRV_HP2620_MOTORS_H__
#define __DRV_HP2620_MOTORS_H__

#include "../new_common.h"
#include "../httpserver/new_http.h"

void HP2620Motors_Init(void);
void HP2620Motors_OnEverySecond(void);
void HP2620Motors_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState);
void HP2620Motors_RunQuickTick(void);
void HP2620Motors_Stop(void);
void HP2620Motors_OnChannelChanged(int ch, int val);

#endif /* __DRV_HP2620_MOTORS_H__ */
