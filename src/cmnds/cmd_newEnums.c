
#include "../logging/logging.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../obk_config.h"
#include "../driver/drv_public.h"
#include <ctype.h>
#include "cmd_local.h"
#include "cmd_newEnums.h"

channelEnum_t **g_enums = 0;

void CMD_FormatEnumTemplate(channelEnum_t *e, char *out, int outSize) {
	char tmp[8];
	*out = 0;
	for (int i = 0; i < e->numOptions; i++) {
		strcat_safe(out, "{% ", outSize);
		if (i == 0) {
			strcat_safe(out, "if", outSize);
		}
		else {
			strcat_safe(out, "elif", outSize);
		}
		strcat_safe(out, " value == '", outSize);
		sprintf(tmp, "%i", e->options[i].value);
		strcat_safe(out, tmp, outSize);
		strcat_safe(out, "' %}\n", outSize);
		strcat_safe(out, "	", outSize);
		strcat_safe(out, e->options[i].label, outSize);
		strcat_safe(out, "\n", outSize);
	}
	strcat_safe(out, "{% else %}\n", outSize);
	strcat_safe(out, "	Unknown\n", outSize);
	strcat_safe(out, "{% endif %}", outSize);
#if WINDOWS
	FILE *f = fopen("lastEnumTemplate.txt", "w");
	fprintf(f, out);
	fclose(f);
#endif
}

commandResult_t CMD_SetChannelEnum(const void *context, const char *cmd,
	const char *args, int cmdFlags) {
	int ch;
	const char *s;

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	ch = Tokenizer_GetArgInteger(0);

	if (g_enums == 0) {
		g_enums = malloc(sizeof(channelEnum_t*)*CHANNEL_MAX);
		memset(g_enums,0, sizeof(channelEnum_t*)*CHANNEL_MAX);
	}
	channelEnum_t *en = malloc(sizeof(channelEnum_t));
	g_enums[ch] = en;
	en->numOptions = Tokenizer_GetArgsCount()-1;
	en->options = malloc(sizeof(channelEnumOption_t)*en->numOptions);
	for (int i = 0; i < en->numOptions; i++) {
		s = Tokenizer_GetArg(1+i);
		en->options[i].value = atoi(s);
		while (*s) {
			if (*s == ':') {
				s++;
				break;
			}
			s++;
		}
		en->options[i].label = strdup(s);
	}
	return CMD_RES_OK;
}

