#include <string.h>
#include "itoa.h"

//WARNING! 
//from https://en.wikibooks.org/wiki/C_Programming/stdlib.h/itoa


 /* reverse:  reverse string s in place */
 char* reverse(char* s)
 {
     int i, j;
     char c;
 
     for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
         c = s[i];
         s[i] = s[j];
         s[j] = c;
     }
     return s;
 }

/* itoa:  convert n to characters in s */
char* itoa(int n, char *s, int radix)
{
    int i, sign;

    if ((sign = n) < 0)  /* record sign */
        n = -n;          /* make n positive */
    do {       /* generate digits in reverse order */
    i = 0;
        s[i++] = n % radix + '0';   /* get next digit */
    } while ((n /= radix) > 0);     /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    return reverse(s);
}

char* ltoa(long n, char *s, int radix)
{
    long i, sign;

    if ((sign = n) < 0)  /* record sign */
        n = -n;          /* make n positive */
    do {       /* generate digits in reverse order */
    i = 0;
        s[i++] = n % radix + '0';   /* get next digit */
    } while ((n /= radix) > 0);     /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    return reverse(s);
}

char* utoa(unsigned n, char *s, int radix)
{
    int i;

    do {       /* generate digits in reverse order */
    i = 0;
        s[i++] = n % radix + '0';   /* get next digit */
    } while ((n /= radix) > 0);     /* delete it */
    s[i] = '\0';
    return reverse(s);
}


char* ultoa(unsigned long n, char *s, int radix)
{
    int i;

    do {       /* generate digits in reverse order */
    i = 0;
        s[i++] = n % radix + '0';   /* get next digit */
    } while ((n /= radix) > 0);     /* delete it */
    s[i] = '\0';
    return reverse(s);
}

