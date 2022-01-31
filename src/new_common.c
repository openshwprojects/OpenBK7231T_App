#include "new_common.h"


// returns amount of space left in buffer (0=overflow happened)
int strcat_safe(char *tg, const char *src, int tgMaxLen) {
	// keep space for 1 more char
	int curOfs = 1;

	// skip
	while(*tg != 0) {
		tg++;
		curOfs++;
		if(curOfs >= tgMaxLen) {
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
		if(curOfs >= tgMaxLen) {
			*tg = 0;
			return 0;
		}
	}
	*tg = 0;
	return tgMaxLen-curOfs;
}


int strcpy_safe(char *tg, const char *src, int tgMaxLen) {
	// keep space for 1 more char
	int curOfs = 1;
	// copy
	while(*src != 0) {
		*tg = *src;
		src++;
		tg++;
		curOfs++;
		if(curOfs >= tgMaxLen) {
			*tg = 0;
			return 0;
		}
	}
	*tg = 0;
	return tgMaxLen-curOfs;
}

void urldecode2_safe(char *dst, const char *src, int maxDstLen)
{
	int curLen = 1;
        char a, b;
        while (*src) {
				if(curLen>=maxDstLen)
					break;
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

