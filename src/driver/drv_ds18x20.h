#ifndef __DRV_DS18X20_H__
#define __DRV_DS18X20_H__

#include "../httpserver/new_http.h"

// Function prototypes
void DS18x20_Init(int pin);
void DS18x20_InitDriver(void);
void DS18x20_ShutdownDriver(void);
void DS18x20_OnEverySecondHook(void);
void DS18x20_AppendInformationToHTTPIndexPageHook(http_request_t* request);

#endif // __DRV_DS18X20_H__
