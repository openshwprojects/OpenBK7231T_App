#pragma once

#include "../httpserver/new_http.h"

void XiaomiCompact4_Init(void);
void XiaomiCompact4_RunEverySecond(void);
void XiaomiCompact4_RunQuickTick(void);
int XiaomiCompact4_ShouldFeedWDT(void);
void XiaomiCompact4_Stop(void);
void XiaomiCompact4_OnChannelChanged(int ch, int value);
void XiaomiCompact4_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState);
