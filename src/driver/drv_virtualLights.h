#pragma once

#include "../httpserver/new_http.h"

void VirtualLights_Init();
void VirtualLights_Shutdown();
void VirtualLights_OnChannelChanged(int ch, int value);
void VirtualLights_OnHassDiscovery(const char *topic);
void VirtualLights_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState);

int VirtualLights_IsEnabled();
int VirtualLights_IsRGBEnabled();
void VirtualLights_SetEnabled(int enabled);
void VirtualLights_SetRGBEnabled(int enabled);
