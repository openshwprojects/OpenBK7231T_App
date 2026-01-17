#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "../hal_pins.h"

// use "complete" search as default, only "simple" one for these platforms
#if !(PLATFORM_LN882H || PLATFORM_W800 || PLATFORM_TXW81X || (PLATFORM_ESPIDF && ! CONFIG_IDF_TARGET_ESP32C3))
// start helpers for finding (su-)string in pinalias

// code to find a pin index by name
// we migth have "complex" or alternate names like
// "IO0/A0" or even "IO2 (B2/TX1)" and would like all to match
// so we need to make sure the substring is found an propperly "terminated"
// by '\0', '/' , '(', ')' or ' '


// Define valid start and end characters for a string (e.g. for "(RX1/IO10)" we would need "()/"
int is_valid_start_end(char ch) {
    // Check if character is in defined terminators
    return strchr(" ()/\0", ch) != NULL;
}

// new version, case insensitive, use 
// wal_stricmp() and (new introduced) wal_stristr()
// from new_common.c
int str_match_in_alias(const char* alias, const char* str) {
    size_t alen = strlen(alias), slen = strlen(str);

    if (slen > alen) return 0; // No match

    // found at start of alisa, no test for a valid "start" 
    if (wal_strnicmp(alias, str, slen) == 0) {
        return (slen == alen || is_valid_start_end(alias[slen]));
    }

    // not at start, so found-1 is a valid position in string
    for (char *found = wal_stristr(alias + 1, str); found; found = wal_stristr(found + 1, str)) {
    
        if (is_valid_start_end(*(found - 1)) && (is_valid_start_end(found[slen]) || found[slen] == '\0')) {
            return 1; // Match found
        }
    }

    return 0; // No match
}



/*
// old version, case sensitive
int str_match_in_alias(const char* alias, const char* str) {
    size_t alias_length = strlen(alias);
    size_t str_length = strlen(str);

    // Early return if `str` (substring) is longer than `alias`
    if (str_length > alias_length) {
        return 0; // No match possible
    }

    // Check if the substring is at the start of the string
    // --> no need (and no possibility) to check previous char
    if (strncmp(alias, str, str_length) == 0) {
        if (str_length < alias_length) {
            char following = alias[str_length];
            if (is_valid_start_end(following)) {
                return 1;  // Match found at the start with valid termination
            }
        } else {
            return 1; // whole string matches
        }
    }

    // try finding str inside alias (not at beginning)
    const char* found = strstr(alias, str);
    while (found != NULL) {
        char previous = *(found - 1);
        if (previous) {  // Ensure previous is valid
            char following = *(found + str_length);
            if (is_valid_start_end(previous) && (following == '\0' || is_valid_start_end(following))) {
                return 1; // Match found at valid boundaries
            }
        }
        found = strstr(found + 1, str);  // Continue searching
    }

    return 0;  // No valid match found
}
*/
// END helpers

int HAL_PIN_Find(const char *name) {
	if (strspn(name,"0123456789") == strlen(name)) {
		return atoi(name);
	}
	for (int i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (str_match_in_alias(HAL_PIN_GetPinNameAlias(i), name)) {
			return i;
		}
	}
	return -1;
}
#else
int HAL_PIN_Find(const char *name) {
	if (strspn(name,"0123456789") == strlen(name)) {
		return atoi(name);
	}
	for (int i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (!stricmp(HAL_PIN_GetPinNameAlias(i), name)) {
			return i;
		}
	}
	return -1;
}

#endif

int __attribute__((weak)) PIN_GetPWMIndexForPinIndex(int pin)
{
	return -1;
}

const char* __attribute__((weak)) HAL_PIN_GetPinNameAlias(int index)
{
	return "error";
}

int __attribute__((weak)) HAL_PIN_CanThisPinBePWM(int index)
{
	return 0;
}

void __attribute__((weak)) HAL_PIN_SetOutputValue(int index, int iVal)
{
	return;
}

int __attribute__((weak)) HAL_PIN_ReadDigitalInput(int index)
{
	return 0;
}

void __attribute__((weak)) HAL_PIN_Setup_Input_Pullup(int index)
{
	return;
}

void __attribute__((weak)) HAL_PIN_Setup_Input_Pulldown(int index)
{
	return;
}

void __attribute__((weak)) HAL_PIN_Setup_Input(int index)
{
	return;
}

void __attribute__((weak)) HAL_PIN_Setup_Output(int index)
{
	return;
}

void __attribute__((weak)) HAL_PIN_PWM_Stop(int index)
{
	return;
}

void __attribute__((weak)) HAL_PIN_PWM_Start(int index, int freq)
{
	return;
}

void __attribute__((weak)) HAL_PIN_PWM_Update(int index, float value)
{
	return;
}

unsigned int __attribute__((weak)) HAL_GetGPIOPin(int index)
{
	return index;
}

void __attribute__((weak)) HAL_AttachInterrupt(int pinIndex, OBKInterruptType mode, OBKInterruptHandler function)
{

}
void __attribute__((weak)) HAL_DetachInterrupt(int pinIndex)
{

}
