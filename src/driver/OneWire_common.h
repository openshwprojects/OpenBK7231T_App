#pragma once
#include "../new_common.h"
#include "../hal/hal_generic.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../cmnds/cmd_public.h"



// some functions used in OneWire / DS18X20 code
// especially timing to implement an own "usleep()"
void OWusleep(int r);
void OWusleepshort(int r);
void OWusleepmed(int r);
void OWusleeplong(int r);

