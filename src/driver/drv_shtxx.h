// drv_shtxx.h  –  SHT3x/SHT4x (Sensirion)
//
// startDriver SHTXX [SDA=<pin>] [SCL=<pin>]
//                   [type=<0|3|4>]     0/omit=auto, 3=SHT3x, 4=SHT4x
//                   [address=<7-bit hex>]
//                   [chan_t=<ch>] [chan_h=<ch>]
//
// Feature gates (define before including or in build flags):
//   SHTXX_ENABLE_SERIAL_LOG               – store + print SHT serial numbers
//   SHTXX_ENABLE_SHT3_EXTENDED_FEATURES   – SHT3x heater/alerts/periodic mode

#ifndef DRV_SHTXX_H
#define DRV_SHTXX_H

#define SHTXX_ENABLE_SERIAL_LOG             1
#define SHTXX_ENABLE_SHT3_EXTENDED_FEATURES 1

#include <stdint.h>
#include <stdbool.h>

#ifndef SHTXX_MAX_SENSORS
#  define SHTXX_MAX_SENSORS 4
#endif

void SHTXX_Init(void);
void SHTXX_StopDriver(void);
void SHTXX_OnEverySecond(void);
void SHTXX_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState);

#endif // DRV_SHTXX_H
