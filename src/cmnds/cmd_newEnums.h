
#include "../logging/logging.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "cmd_local.h"

typedef struct channelEnumOption_s {
	int value;
	char *label;
} channelEnumOption_t;

typedef struct channelEnum_s {
	int numOptions;
	channelEnumOption_t *options;
} channelEnum_t;

extern channelEnum_t **g_enums;


void CMD_FormatEnumTemplate(channelEnum_t *e, char *out, int outSize);




