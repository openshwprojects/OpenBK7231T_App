#pragma once

#include <stdint.h>
#include <time.h>
#include "drv_deviceclock.h"


// Call once at startup
void DCF77_Init(void);

// Call once per second (from your main periodic task)
void DCF77_OnEverySecond(void);

void DCF77_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState);

// Optionally, call on shutdown/reconfig
void DCF77_Stop(void);
void DCF77_Shutdown_Pin(void);


