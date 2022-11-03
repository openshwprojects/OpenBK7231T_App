#include "new_common.h"
#include <stdio.h>
#include <stdlib.h>

const char *str_rssi[] = { "N/A", "Weak", "Fair", "Good", "Excellent" };


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





// Why strdup breaks strings?
// backlog lcd_clearAndGoto I2C1 0x23 1 1; lcd_print I2C1 0x23 Enabled
// it got broken around 64 char
// where is buffer with [64] bytes?
// 2022-11-02 update: It was also causing crash on OpenBL602. Original strdup was crashing while my strdup works.
// Let's just rename test_strdup to strdup and let it be our main correct strdup
#if !defined(PLATFORM_W600) && !defined(PLATFORM_W800)
// W600 and W800 already seem to have a strdup?
char *strdup(const char *s)
{
    char *res;
    size_t len;

    if (s == NULL)
        return NULL;

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

#if 1

void ftoa_fixed(char *buffer, double value);
static void ftoa_sci(char *buffer, double value);

static char *add_char(char *base, char *cur, int max, char ch) {
	int curLen;
	curLen = cur - base;
	if(curLen + 2 >= max)
		return cur;
	*cur = ch;
	cur++;;
	return cur;
}
static char *add_str(char *base, char *cur, int max, const char *s) {
	int curLen;
	curLen = cur - base;
	while(*s) {
		if(curLen + 2 >= max)
			return cur;
		*cur = *s;
		s++;
		cur++;
		curLen++;
	}
	return cur;
}
static int normalize(double *val) {
    int exponent = 0;
    double value = *val;

    while (value >= 1.0) {
        value /= 10.0;
        ++exponent;
    }

    while (value < 0.1) {
        value *= 10.0;
        --exponent;
    }
    *val = value;
    return exponent;
}

void ftoa_fixed(char *buffer, double value){
	int intr = (int)value;
	int ttt = (int)round(value * 1000.0);
	if (ttt < 0) ttt = -ttt;
	ttt = ttt % 1000;

	sprintf(buffer, "%d.%03d", intr, ttt);
}

void ftoa_fixed_x(char *buffer, double value) {  
    /* carry out a fixed conversion of a double value to a string, with a precision of 5 decimal digits. 
     * Values with absolute values less than 0.000001 are rounded to 0.0
     * Note: this blindly assumes that the buffer will be large enough to hold the largest possible result.
     * The largest value we expect is an IEEE 754 double precision real, with maximum magnitude of approximately
     * e+308. The C standard requires an implementation to allow a single conversion to produce up to 512 
     * characters, so that's what we really expect as the buffer size.     
     */

    int exponent = 0;
    int places = 0;
    static const int width = 4;

    if (value == 0.0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }         

    if (value < 0.0) {
        *buffer++ = '-';
        value = -value;
    }

    exponent = normalize(&value);

    while (exponent > 0) {
        int digit = value * 10;
        *buffer++ = digit + '0';
        value = value * 10 - digit;
        ++places;
        --exponent;
    }

    if (places == 0)
        *buffer++ = '0';

    *buffer++ = '.';

    while (exponent < 0 && places < width) {
        *buffer++ = '0';
        --exponent;
        ++places;
    }

    while (places < width) {
        int digit = value * 10.0;
        *buffer++ = digit + '0';
        value = value * 10.0 - digit;
        ++places;
    }
    *buffer = '\0';
}

static void ftoa_sci(char *buffer, double value) {
    int exponent = 0;
    //int places = 0;
    int digit;
	int i;
    static const int width = 4;

    if (value == 0.0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    if (value < 0.0) {
        *buffer++ = '-';
        value = -value;
    }

    exponent = normalize(&value);

    digit = value * 10.0;
    *buffer++ = digit + '0';
    value = value * 10.0 - digit;
    --exponent;

    *buffer++ = '.';

    for (i = 0; i < width; i++) {
        digit = value * 10.0;
        *buffer++ = digit + '0';
        value = value * 10.0 - digit;
    }

    *buffer++ = 'e';
    itoa(exponent, buffer, 10);
}


int vsnprintf2(char *o, size_t olen, char const *fmt, va_list arg) {
	char *baseOut;
    char ch;
    char buffer[16];
	union {
		int int_temp;
		char char_temp;
		char *string_temp;
		double double_temp;
	} u;

	*o = 0;
	baseOut = o;

    while ( (ch = *fmt++) ) {
		u.int_temp = o - baseOut;
		if(u.int_temp + 2 >= olen) {
			break;
		}
        if ( '%' == ch ) {
			if(fmt[0] == '%') {
                /* %% - print out a single %    */
					o = add_char(baseOut,o,olen,'%');

					fmt+=1;
			} else if(fmt[0] == 'c') {
                /* %c: print out a character    */
                    u.char_temp = va_arg(arg, int);
					o = add_char(baseOut,o,olen,u.char_temp);

					fmt+=1;
			} else if(fmt[0] == 's') {
                /* %s: print out a string       */
                   u.string_temp = va_arg(arg, char *);
					
					o = add_str(baseOut,o,olen,u.string_temp);

					fmt+=1;
			} else if(fmt[0] == '0' && fmt[1] == '2' && fmt[3] == 'X') {
              // %02X
                    u.int_temp = va_arg(arg, long int);
                    itoa(u.int_temp, buffer, 16);
					// pad to number of digits
					u.int_temp = (fmt[1] - '0') - strlen(buffer);
					while(u.int_temp > 0) {
						o = add_char(baseOut,o,olen,'0');
						u.int_temp--;
					}

					o = add_str(baseOut,o,olen,buffer);

					fmt+=3;
			} else if(fmt[0] == '0' && (fmt[2] == 'd' || fmt[2] == 'i')) {
              // %02d, %04d
                    u.int_temp = va_arg(arg, long int);
                    itoa(u.int_temp, buffer, 10);
					// pad to number of digits
					u.int_temp = (fmt[1] - '0') - strlen(buffer);
					while(u.int_temp > 0) {
						o = add_char(baseOut,o,olen,'0');
						u.int_temp--;
					}

					o = add_str(baseOut,o,olen,buffer);

					fmt+=3;
			} else if(fmt[0] == '1' && fmt[1] == '.' && fmt[2] == '1' && fmt[3] == 'f') {
              //placeholder for %1.1f Wh<
                    u.double_temp = va_arg(arg, double);
                    ftoa_fixed(buffer, u.double_temp);
					o = add_str(baseOut,o,olen,buffer);

					fmt+=4;
			} else if(fmt[0] == 'l' && fmt[1] == 'd') {
              // %ld, long int
                    u.int_temp = va_arg(arg, long int);
                    itoa(u.int_temp, buffer, 10);
					o = add_str(baseOut,o,olen,buffer);

					fmt+=2;
			} else if(fmt[0] == 'l' && fmt[1] == 'i') {
              // %li, long int
                    u.int_temp = va_arg(arg, long int);
                    itoa(u.int_temp, buffer, 10);
					o = add_str(baseOut,o,olen,buffer);

					fmt+=2;
			} else if(fmt[0] == 'd' || fmt[0] == 'i') {
                /* %d: print out an int         */
                    u.int_temp = va_arg(arg, int);
                    itoa(u.int_temp, buffer, 10);
					o = add_str(baseOut,o,olen,buffer);

					fmt+=1;
			} else if(fmt[0] == 'x') {
                /* %x: print out an int in hex  */
                    u.int_temp = va_arg(arg, int);
                    itoa(u.int_temp, buffer, 16);
					o = add_str(baseOut,o,olen,buffer);

					fmt+=1;
			} else if(fmt[0] == 'f') {
                    u.double_temp = va_arg(arg, double);
                    ftoa_fixed(buffer, u.double_temp);
					o = add_str(baseOut,o,olen,buffer);

					fmt+=1;
			} else if(fmt[0] == 'e') {
                    u.double_temp = va_arg(arg, double);
                    ftoa_sci(buffer, u.double_temp);
					o = add_str(baseOut,o,olen,buffer);

					fmt+=1;
            } else {
					// unknown format
                    u.int_temp = va_arg(arg, int);

			}
        }
        else {
			o = add_char(baseOut,o,olen,ch);
        }
    }
	u.int_temp = o - baseOut;
	baseOut[u.int_temp] = 0;
    return u.int_temp;
}
int snprintf2(char *o, size_t olen, const char* fmt, ...)
{
	int len;
	va_list argList;

	va_start(argList, fmt);
	len = vsnprintf2(o, olen, fmt, argList);
	va_end(argList);
	return len;
}
int sprintf2(char *o, const char* fmt, ...)
{
	int len;
	va_list argList;

	va_start(argList, fmt);
	len = vsnprintf2(o, 8192, fmt, argList);
	va_end(argList);
	return len;
}
#endif