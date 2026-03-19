
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

void CMD_FreeChannelEnumOptions(int ch) {
	if (g_enums && g_enums[ch] && g_enums[ch]->numOptions != 0) {
		for (int i = 0; i < g_enums[ch]->numOptions; i++) {
			os_free(g_enums[ch]->options[i].label);
		}
		os_free(g_enums[ch]->options);
		g_enums[ch]->numOptions=0;
		os_free(g_enums[ch]);
		g_enums[ch] = 0;
	}
}
// method to clean up on shutdown
void CMD_FreeLabels() {
	for (int ch = 0; ch < CHANNEL_MAX; ch++) {
	//	if (g_enums && g_enums[ch] && g_enums[ch]->numOptions != 0) {
		CMD_FreeChannelEnumOptions(ch);
			//os_free(g_enums[ch]);
	}
	//os_free(g_enums);
	//g_enums = 0;

}

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
	int numOptions = 0;
	*out = 0;
	strcat_safe(out, "{{ {", outSize);
	if (e != NULL && e != 0)
		numOptions = e->numOptions;

	for (int i = 0; i < numOptions; i++) {
		if (!isCommand)
			snprintf(tmp, sizeof(tmp), "%i:'%s', ", e->options[i].value,e->options[i].label);
		else
			snprintf(tmp, sizeof(tmp), "'%s':'%i', ", e->options[i].label,e->options[i].value);

		strcat_safe(out, tmp, outSize);
	}
	if (!isCommand) {
		strcat_safe(out, "99999:'Undefined'}", outSize);
		strcat_safe(out,"[(value | int(99999))] | default(\"Undefined Enum [\"~value~\"]\") }}",outSize);
	} else {
		strcat_safe(out, "'Undefined':'99999'}", outSize);
		strcat_safe(out,"[value] | default(99999) }}",outSize);
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
	channelEnum_t *en;

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

	if (g_enums[ch] != 0 && g_enums[ch]->numOptions != 0) {
		// free any previously defined channel enums
		CMD_FreeChannelEnumOptions(ch);
	}
	en = malloc(sizeof(channelEnum_t));
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

		//en->options[i].label = strdup(s);
		int llen = strlen(s) > CMD_ENUM_MAX_LABEL_SIZE ? CMD_ENUM_MAX_LABEL_SIZE : strlen(s);
		label = (char *)malloc(llen+1);
		strncpy(label,s,llen);
		label[llen]='\0';
		en->options[i].label = label;
	}
	return CMD_RES_OK;
}

// helper function to map enum values to labels
char* CMD_FindChannelEnumLabel(channelEnum_t *channelEnum, int value) {
	static char notfound[5]; // assuming INT_MAX 32k
	snprintf(notfound, 5, "%i", value); // if no label defined, use the value
	if (channelEnum == NULL || channelEnum == 0 || channelEnum->numOptions == 0  ) { // g_enum zeroed on init
		return notfound;
	}

	for (int i = 0; i < channelEnum->numOptions; i++) {
		if (channelEnum->options[i].value == value)
			return channelEnum->options[i].label;
	}
	return notfound;
}

