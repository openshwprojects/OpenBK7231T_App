#pragma once

#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"
#include "../hal/hal_pins.h"

//int OWReset(int Pin);
//void OWWriteByte(int Pin, int data);
//int OWReadByte(int Pin);
// int OWConversionDone(int Pin);
int DS1820_getTemp();
void DS1820_driver_Init();
void DS1820_OnEverySecond();
void DS1820_AppendInformationToHTTPIndexPage(http_request_t *request);

