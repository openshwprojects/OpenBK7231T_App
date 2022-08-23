// prevent inclusion of the source code
#define JSMN_HEADER
#include "string.h"
#include "jsmn.h"
#include "../new_common.h"

#define MAX_JSON_VALUE_LENGTH   128

int jsoneq(const char *json, jsmntok_t *tok, const char *s);
bool tryGetTokenString(const char *json, jsmntok_t *tok, char *outBuffer);
