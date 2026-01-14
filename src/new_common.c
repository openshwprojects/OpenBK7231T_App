#include "new_common.h"
#include <stdio.h>
#include <stdlib.h>

const char *str_rssi[] = { "N/A", "Weak", "Fair", "Good", "Excellent" };


#ifdef WIN32

#else

#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 0

// Compile nanoprintf in this translation unit.
#define NANOPRINTF_IMPLEMENTATION

#ifndef ssize_t
#define ssize_t int
#endif

#include "nanoprintf.h"


#ifdef WRAP_PRINTF
#define vsnprintf3 __wrap_vsnprintf
#define snprintf3 __wrap_snprintf
#define sprintf3 __wrap_sprintf
#define vsprintf3 __wrap_vsprintf
#endif

int vsnprintf3(char *buffer, size_t bufsz, const char *fmt, va_list val) {
    int const rv = npf_vsnprintf(buffer, bufsz, fmt, val);
    return rv;
}

int snprintf3(char *buffer, size_t bufsz, const char *fmt, ...) {
   	va_list val;
    va_start(val, fmt);
    int const rv = npf_vsnprintf(buffer, bufsz, fmt, val);
    va_end(val);
    return rv;
}

#define SPRINTFMAX 100
int sprintf3(char *buffer, const char *fmt, ...) {
   	va_list val;
    va_start(val, fmt);
    int const rv = npf_vsnprintf(buffer, SPRINTFMAX, fmt, val);
    va_end(val);
    return rv;
}

int vsprintf3(char *buffer, const char *fmt, va_list val) {
    int const rv = npf_vsnprintf(buffer, SPRINTFMAX, fmt, val);
    return rv;
}

#endif

#if WINDOWS || PLATFORM_W800 || PLATFORM_TXW81X
const char* strcasestr(const char* str1, const char* str2)
{
	const char* p1 = str1;
	const char* p2 = str2;
	const char* r = *p2 == 0 ? str1 : 0;

	while (*p1 != 0 && *p2 != 0)
	{
		if (tolower((unsigned char)*p1) == tolower((unsigned char)*p2))
		{
			if (r == 0)
			{
				r = p1;
			}

			p2++;
		}
		else
		{
			p2 = str2;
			if (r != 0)
			{
				p1 = r + 1;
			}

			if (tolower((unsigned char)*p1) == tolower((unsigned char)*p2))
			{
				r = p1;
				p2++;
			}
			else
			{
				r = 0;
			}
		}

		p1++;
	}

	return *p2 == 0 ? (char*)r : 0;
}
#endif

// Why strdup breaks strings?
// backlog lcd_clearAndGoto I2C1 0x23 1 1; lcd_print I2C1 0x23 Enabled
// it got broken around 64 char
// where is buffer with [64] bytes?
// 2022-11-02 update: It was also causing crash on OpenBL602. Original strdup was crashing while my strdup works.
// Let's just rename test_strdup to strdup and let it be our main correct strdup
#if !defined(PLATFORM_W600) && !defined(PLATFORM_W800) && !defined(WINDOWS) && !defined(PLATFORM_ECR6600) && !PLATFORM_REALTEK_NEW
// W600 and W800 already seem to have a strdup?
char *strdup(const char *s)
{
    char *res;
    size_t len;

    //if (s == NULL)
    //    return NULL;

    len = strlen(s);
    res = malloc(len + 1);
    if (res)
        memcpy(res, s, len + 1);

    return res;
}
#endif

int strIsInteger(const char *s) {
	if(s==0)
		return 0;
	if(*s == 0)
		return 0;
	if(s[0]=='0' && s[1] == 'x'){
		return 1;
	}
	while(*s) {
		if(isdigit((unsigned char)*s)==false)
			return 0;
		s++;
	}
	return 1;
}
// returns amount of space left in buffer (0=overflow happened)
int strcat_safe(char *tg, const char *src, int tgMaxLen) {
	int curOfs = 1;

	// skip
	while(*tg != 0) {
		tg++;
		curOfs++;
		if(curOfs >= tgMaxLen - 1) {
			*tg = 0;
			return 0;
		}
	}
	// copy
	while(*src != 0) {
		*tg = *src;
		src++;
		tg++;
		curOfs++;
		if(curOfs >= tgMaxLen - 1) {
			*tg = 0;
			return 0;
		}
	}
	*tg = 0;
	return tgMaxLen-curOfs;
}


int strcpy_safe_checkForChanges(char *tg, const char *src, int tgMaxLen) {
	int changesFound = 0;
	int curOfs = 0;
	// copy
	while(*src != 0) {
		if(*tg != *src) {
			changesFound++;
			*tg = *src;
		}
		src++;
		tg++;
		curOfs++;
		if(curOfs >= tgMaxLen - 1) {
			if(*tg != 0) {
				changesFound++;
				*tg = 0;
			}
			return 0;
		}
	}
	if(*tg != 0) {
		changesFound++;
		*tg = 0;
	}
	return changesFound;
}
int strcpy_safe(char *tg, const char *src, int tgMaxLen) {
	int curOfs = 0;
	// copy
	while(*src != 0) {
		*tg = *src;
		src++;
		tg++;
		curOfs++;
		if(curOfs >= tgMaxLen - 1) {
			*tg = 0;
			return 0;
		}
	}
	*tg = 0;
	return tgMaxLen-curOfs;
}

void urldecode2_safe(char *dst, const char *srcin, int maxDstLen)
{
	int curLen = 1;
        int a = 0, b = 0;
	// avoid signing issues in conversion to int for isxdigit(int c)
	const unsigned char *src = (const unsigned char *)srcin;
        while (*src) {
		if(curLen >= (maxDstLen - 1)){
			break;
		}
                if ((*src == '%') &&
                    ((a = src[1]) && (b = src[2])) &&
                    (isxdigit(a) && isxdigit(b))) {
                        if (a >= 'a')
                                a -= 'a'-'A';
                        if (a >= 'A')
                                a -= ('A' - 10);
                        else
                                a -= '0';
                        if (b >= 'a')
                                b -= 'a'-'A';
                        if (b >= 'A')
                                b -= ('A' - 10);
                        else
                                b -= '0';
                        *dst++ = 16*a+b;
                        src+=3;
                } else if (*src == '+') {
                        *dst++ = ' ';
                        src++;
                } else {
                        *dst++ = *src++;
                }
		curLen++;
        }
        *dst++ = '\0';
}

void stripDecimalPlaces(char *p, int maxDecimalPlaces) {
	while (1) {
		if (*p == '.')
			break;
		if (*p == 0)
			return;
		p++;
	}
	if (maxDecimalPlaces == 0) {
		*p = 0;
		return;
	}
	p++;
	while (maxDecimalPlaces > 0) {
		if (*p == 0)
			return;
		maxDecimalPlaces--;
		p++;
	}
	*p = 0;
}
int wal_stricmp(const char* a, const char* b) {
	int ca, cb;
	do {
		ca = (unsigned char)*a++;
		cb = (unsigned char)*b++;
		ca = tolower(toupper(ca));
		cb = tolower(toupper(cb));
	} while ((ca == cb) && (ca != '\0'));
	return ca - cb;
}
int wal_strnicmp(const char* a, const char* b, int count) {
	int ca, cb;
	do {
		ca = (unsigned char)*a++;
		cb = (unsigned char)*b++;
		ca = tolower(toupper(ca));
		cb = tolower(toupper(cb));
		count--;
	} while ((ca == cb) && (ca != '\0') && (count > 0));
	return ca - cb;
}

const char* skipToNextWord(const char* p) {
	while (isWhiteSpace(*p) == false) {
		if (*p == 0)
			return p;
		p++;
	}
	while (isWhiteSpace(*p)) {
		if (*p == 0)
			return p;
		p++;
	}
	return p;
}

int STR_ReplaceWhiteSpacesWithUnderscore(char *p) {
	int r = 0;
	while (*p) {
		bool bSpecialChar = false;
		if (*((byte*)p) > 127) {
			bSpecialChar = true;
		}
		if (*p == ' ' || *p == '\t' || bSpecialChar) {
			r++;
			*p = '_';
		}
		p++;
	}
	return r;
}


WIFI_RSSI_LEVEL wifi_rssi_scale(int8_t rssi_value)
{
    #define LEVEL_WEAK      -70     //-70
    #define LEVEL_FAIR      -60     //-60
    #define LEVEL_GOOD      -50     //-50

    WIFI_RSSI_LEVEL retVal = NOT_CONNECTED;

    if (rssi_value <= LEVEL_WEAK)
        retVal = WEAK;
    else if ((rssi_value <= LEVEL_FAIR) && (rssi_value > LEVEL_WEAK))
        retVal = FAIR;
    else if ((rssi_value <= LEVEL_GOOD) && (rssi_value > LEVEL_FAIR))
        retVal = GOOD; 
    else if (rssi_value > LEVEL_GOOD)
        retVal = EXCELLENT;
    return retVal;
}

