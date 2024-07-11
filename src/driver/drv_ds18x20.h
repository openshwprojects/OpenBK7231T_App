#ifndef __DRV_DS18X20_H__
#define __DRV_DS18X20_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "../httpserver/new_http.h"

// Function prototypes
void DS18x20_Init(int pin);
void DS18x20_InitDriver(void);
void DS18x20_ShutdownDriver(void);
void DS18x20_OnEverySecondHook(void);
void DS18x20_AppendInformationToHTTPIndexPageHook(http_request_t* request);

#ifdef __cplusplus
}
#endif

#endif // __DRV_DS18X20_H__
