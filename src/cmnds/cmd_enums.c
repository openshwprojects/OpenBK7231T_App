
#include "../logging/logging.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../obk_config.h"
#include "../driver/drv_public.h"
#include <ctype.h>
#include "cmd_local.h"
#include "cmd_enums.h"

#define CMD_ENUM_MAX_LABEL_SIZE 32

channelEnum_t **g_enums = 0;


// helpers for preparing a homeassistant select template for enums
void CMD_GenEnumValueTemplate(channelEnum_t *e, char *out, int outSize) {
	CMD_FormatEnumTemplate(e, out, outSize, false);
}
void CMD_GenEnumCommandTemplate(channelEnum_t *e, char *out, int outSize) {
	CMD_FormatEnumTemplate(e, out, outSize, true);
}

// this is quite similar to hass.c generate_command_template,
// except for enum types are not ordered and don't align to an array index
// we also want a default for undefined enums
// in the future this could be merge with those in hass.c
void CMD_FormatEnumTemplate(channelEnum_t *e, char *out,
	int outSize, bool isCommand) {
	char tmp[CMD_ENUM_MAX_LABEL_SIZE+24];
	*out = 0;
	strcat_safe(out, "{% set mapper = { ", outSize);
	for (int i = 0; i < e->numOptions; i++) {
		if (!isCommand)
			sprintf(tmp, "%i: '%s', ", e->options[i].value,e->options[i].label);
		else
			sprintf(tmp, "'%s': '%i', ", e->options[i].label,e->options[i].value);

		strcat_safe(out, tmp, outSize);
	}
	if (!isCommand) {
		strcat_safe(out, "99999: 'Undefined'} %}", outSize);
		strcat_safe(out,"{{ mapper[(value | int(99999))] | default(\"Undefined\")}}",outSize);
	} else {
		strcat_safe(out, "'Undefined': '99999'} %}", outSize);
		strcat_safe(out,"{{ (mapper[value] | default(99999)) }}",outSize);
	}

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
	char *label;

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
	channelEnum_t *en;

	if (g_enums[ch] != 0 && g_enums[ch]->numOptions != 0) {
		// free any previously defined channel enums
		en = g_enums[ch];
		for (int i = 0; i< en->numOptions; i++) {
			os_free(en->options[i].label);
		}
	} else {
		en = malloc(sizeof(channelEnum_t));
		g_enums[ch] = en;
	}
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

		//en->options[i].label = strdup(s);
		int llen = strlen(s) > CMD_ENUM_MAX_LABEL_SIZE ? CMD_ENUM_MAX_LABEL_SIZE : strlen(s);
		label = (char *)malloc(llen+1);
		strncpy(label,s,llen);
		label[llen]='\0';
		en->options[i].label = label;
		os_free(s);
	}
	return CMD_RES_OK;
}

// helper function to map enum values to labels
char* CMD_FindChannelEnumLabel(channelEnum_t *channelEnum, int value) {
	char* notfound = (char*)malloc(sizeof(char) * 5); // assuming INT_MAX 32k
	snprintf(notfound, 5, "%i", value); // if no label defined, use the value
	if (channelEnum == NULL || channelEnum->numOptions == 0  ) { // g_enum zeroed on init
		return notfound;
	}

	for (int i = 0; i < channelEnum->numOptions; i++) {
		if (channelEnum->options[i].value == value)
			return channelEnum->options[i].label;
	}
	return notfound;
}
