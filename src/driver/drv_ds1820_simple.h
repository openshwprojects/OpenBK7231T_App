#pragma once

#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "../httpserver/new_http.h"


int DS1820_getTemp();
void DS1820_driver_Init();
void DS1820_OnEverySecond();
void DS1820_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState);

