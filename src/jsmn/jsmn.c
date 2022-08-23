
#include "string.h"
// this includes the source code.
#include "jsmn.h"
#include "jsmn_h.h"

int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}

/* Extracts string token value into outBuffer (128 char). Returns true if the operation was successful. */
bool tryGetTokenString(const char *json, jsmntok_t *tok, char *outBuffer){
  if (tok == NULL || tok->type != JSMN_STRING){
    return false;
  }
  
  int length = tok->end - tok->start;

  //Don't have enough buffer
  if (length > MAX_JSON_VALUE_LENGTH) {
    return false;
  }

  memset(outBuffer, '\0', MAX_JSON_VALUE_LENGTH); //Wipe previous value
  strncpy(outBuffer, json + tok->start, length);
  return true;
}