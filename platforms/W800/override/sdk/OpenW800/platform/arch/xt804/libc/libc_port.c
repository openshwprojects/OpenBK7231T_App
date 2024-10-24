/*
 * Copyright (C) 2017 C-SKY Microsystems Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/******************************************************************************
 * @file     libc_port.c
 * @brief    libc port
 * @version  V1.0
 * @date     26. Dec 2017
 ******************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <limits.h>
#include "stddef.h"
#include <csi_config.h>
#include "wm_regs.h"

static unsigned char printf_port = 0;
//0 uart0; 1: uart1; other: close;
unsigned char get_printf_port(void)
{
    return printf_port;
}

void set_printf_port(unsigned char port)
{
    if(port == 0)
    {
        printf_port = 0;
    }
    else if(port == 1)
    {
        printf_port = 1;
    }
    else
    {
        printf_port = 0xff;
    }
}

int sendchar(int ch)
{
	//tls_reg_write32(HR_UART0_INT_MASK, 0x3);
	if (printf_port == 0)
	{
		if(ch == '\n')
		{
			while (tls_reg_read32(HR_UART0_FIFO_STATUS) & 0x3F);
			tls_reg_write32(HR_UART0_TX_WIN, '\r');
	    }
	    while(tls_reg_read32(HR_UART0_FIFO_STATUS) & 0x3F);
	    tls_reg_write32(HR_UART0_TX_WIN, (char)ch);
	}
	else if (printf_port == 1)
	{
		if(ch == '\n')
		{
			while (tls_reg_read32(HR_UART1_FIFO_STATUS) & 0x3F);
			tls_reg_write32(HR_UART1_TX_WIN, '\r');
	    }
		while(tls_reg_read32(HR_UART1_FIFO_STATUS) & 0x3F);
		tls_reg_write32(HR_UART1_TX_WIN, (char)ch);
	}
    //tls_reg_write32(HR_UART0_INT_MASK, 0x0);
    return ch;
}

int fputc(int ch, FILE *stream)
{
    (void)stream;
    sendchar(ch);
    return 0;
}

#if 1//defined(__MINILIBC__)
int fgetc(FILE *stream)
{
    (void)stream;

    return 0;
}
#endif

#if 1//defined(_NEWLIB_VERSION_H__)
//_ssize_t _write_r (struct _reent *, int, const void *, size_t) /* ignore warning */
int _write_r(void *r, int file, const void *ptr, size_t len)
{
	size_t i;
	char *p;

	p = (char*) ptr;
	
	for (i = 0; i < len; i++) 
	{
		(void)fputc(*p++, r); /* r: ignore warning */
	}
	return len;
}
#endif

static int __ip2str(unsigned char v4v6, unsigned int *inuint, char *outtxt)
{
    unsigned char i;
    unsigned char j = 0;
    unsigned char k;
    unsigned char h;
    unsigned char m;
    unsigned char l;
    unsigned char bit;

    if (4 == v4v6)
    {
        for(i = 0; i < 4; i++)
        {
            bit = (*inuint >> (8 * i)) & 0xff;
            h = bit / 100;
            if (h)
                outtxt[j++] = '0' + h;
            m = (bit % 100) / 10;
            if (m)
            {
                outtxt[j++] = '0' + m;
            }
            else
            {
                if (h)
                    outtxt[j++] = '0';
            }
            l = (bit % 100) % 10;
            outtxt[j++] = '0' + l;
            outtxt[j++] = '.';
        }
    }
    else
    {
        for (k = 0; k < 4; k++)
        {
            for(i = 0; i < 4; i++)
            {
                m = (*inuint >> (8 * i)) & 0xff;
                h = m >> 4;
                l = m & 0xf;
                if (h > 9)
                    outtxt[j++] = 'A' + h - 10;
                else 
                    outtxt[j++]= '0' + h;
                if (l > 9)
                    outtxt[j++] = 'A' + l - 10;
                else
                    outtxt[j++] = '0' + l;
                if (0 != (i % 2))
                    outtxt[j++] = ':';
            }
            inuint++;
        }
    }

    outtxt[j - 1] = 0;
    return j - 1;
}

static int __mac2str(unsigned char *inchar, char *outtxt)
{
    unsigned char hbit,lbit;
    unsigned int i;

    for(i = 0; i < 6; i++)/* mac length */
    {
        hbit = (*(inchar + i) & 0xf0) >> 4;
        lbit = *(inchar + i ) & 0x0f;
        if (hbit > 9)
            outtxt[3 * i] = 'A' + hbit - 10;
        else 
            outtxt[3 * i]= '0' + hbit;
        if (lbit > 9)
            outtxt[3 * i + 1] = 'A' + lbit - 10;
        else
            outtxt[3 * i + 1] = '0' + lbit;
        outtxt[3 * i + 2] = '-';
    }

    outtxt[3 * (i - 1) + 2] = 0;

    return 3 * (i - 1) + 2;
}

static inline int digitval(int ch)
{
  if ( ch >= '0' && ch <= '9' ) {
    return ch-'0';
  } else if ( ch >= 'A' && ch <= 'Z' ) {
    return ch-'A'+10;
  } else if ( ch >= 'a' && ch <= 'z' ) {
    return ch-'a'+10;
  } else {
    return -1;
  }
}

uintmax_t strntoumax(const char *nptr, char **endptr, int base, size_t n)
{
  int minus = 0;
  uintmax_t v = 0;
  int d;

  while ( n && isspace((unsigned char)*nptr) ) {
    nptr++;
    n--;
  }

  /* Single optional + or - */
  if ( n && *nptr == '-' ) {
    minus = 1;
    nptr++;
    n--;
  } else if ( n && *nptr == '+' ) {
    nptr++;
  }

  if ( base == 0 ) {
    if ( n >= 2 && nptr[0] == '0' &&
	 (nptr[1] == 'x' || nptr[1] == 'X') ) {
      n -= 2;
      nptr += 2;
      base = 16;
    } else if ( n >= 1 && nptr[0] == '0' ) {
      n--;
      nptr++;
      base = 8;
    } else {
      base = 10;
    }
  } else if ( base == 16 ) {
    if ( n >= 2 && nptr[0] == '0' &&
	 (nptr[1] == 'x' || nptr[1] == 'X') ) {
      n -= 2;
      nptr += 2;
    }
  }

  while ( n && (d = digitval(*nptr)) >= 0 && d < base ) {
    v = v*base + d;
    n--;
    nptr++;
  }

  if ( endptr )
    *endptr = (char *)nptr;

  return minus ? -v : v;
}

#ifndef LONG_BIT
#define LONG_BIT (CHAR_BIT*sizeof(long))
#endif

enum flags {
  FL_SPLAT  = 0x01,		/* Drop the value, do not assign */
  FL_INV    = 0x02,		/* Character-set with inverse */
  FL_WIDTH  = 0x04,		/* Field width specified */
  FL_MINUS  = 0x08,		/* Negative number */
};

enum ranks {
  rank_char	= -2,
  rank_short	= -1,
  rank_int 	= 0,
  rank_long	= 1,
  rank_longlong	= 2,
  rank_ptr      = INT_MAX	/* Special value used for pointers */
};

#define MIN_RANK	rank_char
#define MAX_RANK	rank_longlong

#define INTMAX_RANK	rank_longlong
#define SIZE_T_RANK	rank_long
#define PTRDIFF_T_RANK	rank_long

enum bail {
  bail_none = 0,		/* No error condition */
  bail_eof,			/* Hit EOF */
  bail_err			/* Conversion mismatch */
};

static inline const char *
skipspace(const char *p)
{
  while ( isspace((unsigned char)*p) ) p++;
  return p;
}

#undef set_bit
static inline void
set_bit(unsigned long *bitmap, unsigned int bit)
{
  bitmap[bit/LONG_BIT] |= 1UL << (bit%LONG_BIT);
}

#undef test_bit
static inline int
test_bit(unsigned long *bitmap, unsigned int bit)
{
  return (int)(bitmap[bit/LONG_BIT] >> (bit%LONG_BIT)) & 1;
}

int wm_vsscanf(const char *buffer, const char *format, va_list ap)
{
  const char *p = format;
  char ch;
  const char *q = buffer;
  const char *qq;
  uintmax_t val = 0;
  int rank = rank_int;		/* Default rank */
  unsigned int width = UINT_MAX;
  int base;
  enum flags flags = 0;
  enum {
    st_normal,			/* Ground state */
    st_flags,			/* Special flags */
    st_width,			/* Field width */
    st_modifiers,		/* Length or conversion modifiers */
    st_match_init,		/* Initial state of %[ sequence */
    st_match,			/* Main state of %[ sequence */
    st_match_range,		/* After - in a %[ sequence */
  } state = st_normal;
  char *sarg = NULL;		/* %s %c or %[ string argument */
  enum bail bail = bail_none;
  int sign = 0;
  int converted = 0;		/* Successful conversions */
  unsigned long matchmap[((1 << CHAR_BIT)+(LONG_BIT-1))/LONG_BIT];
  int matchinv = 0;		/* Is match map inverted? */
  unsigned char range_start = 0;

  sign = sign; /* gcc warning */

  while ( (ch = *p++) && !bail ) {
    switch ( state ) {
    case st_normal:
      if ( ch == '%' ) {
	state = st_flags;
	flags = 0; rank = rank_int; width = UINT_MAX;
      } else if ( isspace((unsigned char)ch) ) {
	q = skipspace(q);
      } else {
	if ( *q == ch )
	  q++;
	else
	  bail = bail_err;	/* Match failure */
      }
      break;

    case st_flags:
      switch ( ch ) {
      case '*':
	flags |= FL_SPLAT;
	break;
      case '0' ... '9':
	width = (ch-'0');
	state = st_width;
	flags |= FL_WIDTH;
	break;
      default:
	state = st_modifiers;
	p--;			/* Process this character again */
	break;
      }
      break;

    case st_width:
      if ( ch >= '0' && ch <= '9' ) {
	width = width*10+(ch-'0');
      } else {
	state = st_modifiers;
	p--;			/* Process this character again */
      }
      break;

    case st_modifiers:
      switch ( ch ) {
	/* Length modifiers - nonterminal sequences */
      case 'h':
	rank--;			/* Shorter rank */
	break;
      case 'l':
	rank++;			/* Longer rank */
	break;
      case 'j':
	rank = INTMAX_RANK;
	break;
      case 'z':
	rank = SIZE_T_RANK;
	break;
      case 't':
	rank = PTRDIFF_T_RANK;
	break;
      case 'L':
      case 'q':
	rank = rank_longlong;	/* long double/long long */
	break;

      default:
	/* Output modifiers - terminal sequences */
	state = st_normal;	/* Next state will be normal */
	if ( rank < MIN_RANK )	/* Canonicalize rank */
	  rank = MIN_RANK;
	else if ( rank > MAX_RANK )
	  rank = MAX_RANK;

	switch ( ch ) {
	case 'P':		/* Upper case pointer */
	case 'p':		/* Pointer */
#if 0	/* Enable this to allow null pointers by name */
	  q = skipspace(q);
	  if ( !isdigit((unsigned char)*q) ) {
	    static const char * const nullnames[] =
	    { "null", "nul", "nil", "(null)", "(nul)", "(nil)", 0 };
	    const char * const *np;

	    /* Check to see if it's a null pointer by name */
	    for ( np = nullnames ; *np ; np++ ) {
	      if ( !strncasecmp(q, *np, strlen(*np)) ) {
		val = (uintmax_t)((void *)NULL);
		goto set_integer;
	      }
	    }
	    /* Failure */
	    bail = bail_err;
	    break;
	  }
	  /* else */
#endif
	  rank = rank_ptr;
	  base = 0; sign = 0;
	  goto scan_int;

	case 'i':		/* Base-independent integer */
	  base = 0; sign = 1;
	  goto scan_int;

	case 'd':		/* Decimal integer */
	  base = 10; sign = 1;
	  goto scan_int;

	case 'o':		/* Octal integer */
	  base = 8; sign = 0;
	  goto scan_int;

	case 'u':		/* Unsigned decimal integer */
	  base = 10; sign = 0;
	  goto scan_int;
	  
	case 'x':		/* Hexadecimal integer */
	case 'X':
	  base = 16; sign = 0;
	  goto scan_int;

	case 'n':		/* Number of characters consumed */
	  val = (q-buffer);
	  goto set_integer;

	scan_int:
	  q = skipspace(q);
	  if ( !*q ) {
	    bail = bail_eof;
	    break;
	  }
	  val = strntoumax(q, (char **)&qq, base, width);
	  if ( qq == q ) {
	    bail = bail_err;
	    break;
	  }
	  q = qq;
	  converted++;
	  /* fall through */

	set_integer:
	  if ( !(flags & FL_SPLAT) ) {
	    switch(rank) {
	    case rank_char:
	      *va_arg(ap, unsigned char *) = (unsigned char)val;
	      break;
	    case rank_short:
	      *va_arg(ap, unsigned short *) = (unsigned short)val;
	      break;
	    case rank_int:
	      *va_arg(ap, unsigned int *) = (unsigned int)val;
	      break;
	    case rank_long:
	      *va_arg(ap, unsigned long *) = (unsigned long)val;
	      break;
	    case rank_longlong:
	      *va_arg(ap, unsigned long long *) = (unsigned long long)val;
	      break;
	    case rank_ptr:
	      *va_arg(ap, void **) = (void *)(uintptr_t)val;
	      break;
	    }
	  }
	  break;
	  
	case 'c':               /* Character */
          width = (flags & FL_WIDTH) ? width : 1; /* Default width == 1 */
          sarg = va_arg(ap, char *);
          while ( width-- ) {
            if ( !*q ) {
              bail = bail_eof;
              break;
            }
            *sarg++ = *q++;
          }
          if ( !bail )
            converted++;
          break;

        case 's':               /* String */
	  {
	    char *sp;
	    sp = sarg = va_arg(ap, char *);
	    while ( width-- && *q && !isspace((unsigned char)*q) ) {
	      *sp++ = *q++;
	    }
	    if ( sarg != sp ) {
	      *sp = '\0';	/* Terminate output */
	      converted++;
	    } else {
	      bail = bail_eof;
	    }
	  }
	  break;

	  case 'f':               /* float */
	  {
        float *vp = (float *) va_arg(ap, float *);
        const char *vpq = q;
        *vp = strtof(vpq, (char **)&q);
	    if ( vpq != q ) {
	      converted++;
	    } else {
	      //bail = bail_eof;
	      bail = bail_err;
	    }
	  }
	  break;
	  case 'g':               /* double */
	  {
        double *vp = (double *) va_arg(ap, double *);
        const char *vpq = q;
        *vp = strtod(vpq, (char **)&q);
	    if ( vpq != q ) {
	      converted++;
	    } else {
	      //bail = bail_eof;
	      bail = bail_err;
	    }
	  }
	  break;
	  
	case '[':		/* Character range */
	  sarg = va_arg(ap, char *);
	  state = st_match_init;
	  matchinv = 0;
	  memset(matchmap, 0, sizeof matchmap);
	  break;

	case '%':		/* %% sequence */
	  if ( *q == '%' )
	    q++;
	  else
	    bail = bail_err;
	  break;

	default:		/* Anything else */
	  bail = bail_err;	/* Unknown sequence */
	  break;
	}
      }
      break;
    
    case st_match_init:		/* Initial state for %[ match */
      if ( ch == '^' && !(flags & FL_INV) ) {
	matchinv = 1;
      } else {
	set_bit(matchmap, (unsigned char)ch);
	state = st_match;
      }
      break;
      
    case st_match:		/* Main state for %[ match */
      if ( ch == ']' ) {
	goto match_run;
      } else if ( ch == '-' ) {
	range_start = (unsigned char)ch;
	state = st_match_range;
      } else {
	set_bit(matchmap, (unsigned char)ch);
      }
      break;
      
    case st_match_range:		/* %[ match after - */
      if ( ch == ']' ) {
	set_bit(matchmap, (unsigned char)'-'); /* - was last character */
	goto match_run;
      } else {
	int i;
	for ( i = range_start ; i < (unsigned char)ch ; i++ )
	  set_bit(matchmap, i);
	state = st_match;
      }
      break;

    match_run:			/* Match expression finished */
      qq = q;
      while ( width && *q && test_bit(matchmap, (unsigned char)*q)^matchinv ) {
	*sarg++ = *q++;
      }
      if ( q != qq ) {
	*sarg = '\0';
	converted++;
      } else {
	bail = *q ? bail_err : bail_eof;
      }
      break;
    }
  }

  if ( bail == bail_eof && !converted )
    converted = -1;		/* Return EOF (-1) */

  return converted;
}


#define PRINTF_NTOA_BUFFER_SIZE    32U
#define PRINTF_FTOA_BUFFER_SIZE    32U
#define PRINTF_SUPPORT_FLOAT
#define PRINTF_SUPPORT_EXPONENTIAL
#define PRINTF_DEFAULT_FLOAT_PRECISION  6U
#define PRINTF_MAX_FLOAT  1e9
#define PRINTF_SUPPORT_LONG_LONG
#define PRINTF_SUPPORT_PTRDIFF_T


// import float.h for DBL_MAX
#if defined(PRINTF_SUPPORT_FLOAT)
#include <float.h>
#endif


// internal flag definitions
#define FLAGS_ZEROPAD   (1U <<  0U)
#define FLAGS_LEFT      (1U <<  1U)
#define FLAGS_PLUS      (1U <<  2U)
#define FLAGS_SPACE     (1U <<  3U)
#define FLAGS_HASH      (1U <<  4U)
#define FLAGS_UPPERCASE (1U <<  5U)
#define FLAGS_CHAR      (1U <<  6U)
#define FLAGS_SHORT     (1U <<  7U)
#define FLAGS_LONG      (1U <<  8U)
#define FLAGS_LONG_LONG (1U <<  9U)
#define FLAGS_PRECISION (1U << 10U)
#define FLAGS_ADAPT_EXP (1U << 11U)

// output function type
typedef void (*out_fct_type)(char character, void* buffer, size_t idx, size_t maxlen);

// internal buffer output
static inline void _out_buffer(char character, void* buffer, size_t idx, size_t maxlen)
{
  if (idx < maxlen) {
    ((char*)buffer)[idx] = character;
  }
}

static inline void _out_uart(char character, void* buffer, size_t idx, size_t maxlen)
{
  _write_r(NULL, 0, &character, 1);
}

// internal null output
static inline void _out_null(char character, void* buffer, size_t idx, size_t maxlen)
{
  (void)character; (void)buffer; (void)idx; (void)maxlen;
}

// internal secure strlen
// \return The length of the string (excluding the terminating 0) limited by 'maxsize'
static inline unsigned int _strnlen_s(const char* str, size_t maxsize)
{
  const char* s;
  for (s = str; *s && maxsize--; ++s);
  return (unsigned int)(s - str);
}

// internal test if char is a digit (0-9)
// \return true if char is a digit
static inline bool _is_digit(char ch)
{
  return (ch >= '0') && (ch <= '9');
}

// internal ASCII string to unsigned int conversion
static unsigned int _atoi(const char** str)
{
  unsigned int i = 0U;
  while (_is_digit(**str)) {
    i = i * 10U + (unsigned int)(*((*str)++) - '0');
  }
  return i;
}

// output the specified string in reverse, taking care of any zero-padding
static size_t _out_rev(out_fct_type out, char* buffer, size_t idx, size_t maxlen, const char* buf, size_t len, unsigned int width, unsigned int flags)
{
  const size_t start_idx = idx;

  // pad spaces up to given width
  if (!(flags & FLAGS_LEFT) && !(flags & FLAGS_ZEROPAD)) {
    for (size_t i = len; i < width; i++) {
      out(' ', buffer, idx++, maxlen);
    }
  }

  // reverse string
  while (len) {
    out(buf[--len], buffer, idx++, maxlen);
  }

  // append pad spaces up to given width
  if (flags & FLAGS_LEFT) {
    while (idx - start_idx < width) {
      out(' ', buffer, idx++, maxlen);
    }
  }

  return idx;
}

// internal itoa format
static size_t _ntoa_format(out_fct_type out, char* buffer, size_t idx, size_t maxlen, char* buf, size_t len, bool negative, unsigned int base, unsigned int prec, unsigned int width, unsigned int flags)
{
  // pad leading zeros
  if (!(flags & FLAGS_LEFT)) {
    if (width && (flags & FLAGS_ZEROPAD) && (negative || (flags & (FLAGS_PLUS | FLAGS_SPACE)))) {
      width--;
    }
    while ((len < prec) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
      buf[len++] = '0';
    }
    while ((flags & FLAGS_ZEROPAD) && (len < width) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
      buf[len++] = '0';
    }
  }

  // handle hash
  if (flags & FLAGS_HASH) {
    if (!(flags & FLAGS_PRECISION) && len && ((len == prec) || (len == width))) {
      len--;
      if (len && (base == 16U)) {
        len--;
      }
    }
    if ((base == 16U) && !(flags & FLAGS_UPPERCASE) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
      buf[len++] = 'x';
    }
    else if ((base == 16U) && (flags & FLAGS_UPPERCASE) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
      buf[len++] = 'X';
    }
    else if ((base == 2U) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
      buf[len++] = 'b';
    }
    if (len < PRINTF_NTOA_BUFFER_SIZE) {
      buf[len++] = '0';
    }
  }

  if (len < PRINTF_NTOA_BUFFER_SIZE) {
    if (negative) {
      buf[len++] = '-';
    }
    else if (flags & FLAGS_PLUS) {
      buf[len++] = '+';  // ignore the space if the '+' exists
    }
    else if (flags & FLAGS_SPACE) {
      buf[len++] = ' ';
    }
  }

  return _out_rev(out, buffer, idx, maxlen, buf, len, width, flags);
}


// internal itoa for 'long' type
static size_t _ntoa_long(out_fct_type out, char* buffer, size_t idx, size_t maxlen, unsigned long value, bool negative, unsigned long base, unsigned int prec, unsigned int width, unsigned int flags)
{
  char buf[PRINTF_NTOA_BUFFER_SIZE];
  size_t len = 0U;

  // no hash for 0 values
  if (!value) {
    flags &= ~FLAGS_HASH;
  }

  // write if precision != 0 and value is != 0
  if (!(flags & FLAGS_PRECISION) || value) {
    do {
      const char digit = (char)(value % base);
      buf[len++] = digit < 10 ? '0' + digit : (flags & FLAGS_UPPERCASE ? 'A' : 'a') + digit - 10;
      value /= base;
    } while (value && (len < PRINTF_NTOA_BUFFER_SIZE));
  }

  return _ntoa_format(out, buffer, idx, maxlen, buf, len, negative, (unsigned int)base, prec, width, flags);
}


// internal itoa for 'long long' type
#if defined(PRINTF_SUPPORT_LONG_LONG)
static size_t _ntoa_long_long(out_fct_type out, char* buffer, size_t idx, size_t maxlen, unsigned long long value, bool negative, unsigned long long base, unsigned int prec, unsigned int width, unsigned int flags)
{
  char buf[PRINTF_NTOA_BUFFER_SIZE];
  size_t len = 0U;

  // no hash for 0 values
  if (!value) {
    flags &= ~FLAGS_HASH;
  }

  // write if precision != 0 and value is != 0
  if (!(flags & FLAGS_PRECISION) || value) {
    do {
      const char digit = (char)(value % base);
      buf[len++] = digit < 10 ? '0' + digit : (flags & FLAGS_UPPERCASE ? 'A' : 'a') + digit - 10;
      value /= base;
    } while (value && (len < PRINTF_NTOA_BUFFER_SIZE));
  }

  return _ntoa_format(out, buffer, idx, maxlen, buf, len, negative, (unsigned int)base, prec, width, flags);
}
#endif  // PRINTF_SUPPORT_LONG_LONG

#if defined(PRINTF_SUPPORT_FLOAT)

#if defined(PRINTF_SUPPORT_EXPONENTIAL)
// forward declaration so that _ftoa can switch to exp notation for values > PRINTF_MAX_FLOAT
static size_t _etoa(out_fct_type out, char* buffer, size_t idx, size_t maxlen, double value, unsigned int prec, unsigned int width, unsigned int flags);
#endif


// internal ftoa for fixed decimal floating point
static size_t _ftoa(out_fct_type out, char* buffer, size_t idx, size_t maxlen, double value, unsigned int prec, unsigned int width, unsigned int flags)
{
  char buf[PRINTF_FTOA_BUFFER_SIZE];
  size_t len  = 0U;
  double diff = 0.0;

  // powers of 10
  static const double pow10[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 };

  // test for special values
  if (value != value)
    return _out_rev(out, buffer, idx, maxlen, "nan", 3, width, flags);
  if (value < -DBL_MAX)
    return _out_rev(out, buffer, idx, maxlen, "fni-", 4, width, flags);
  if (value > DBL_MAX)
    return _out_rev(out, buffer, idx, maxlen, (flags & FLAGS_PLUS) ? "fni+" : "fni", (flags & FLAGS_PLUS) ? 4U : 3U, width, flags);

  // test for very large values
  // standard printf behavior is to print EVERY whole number digit -- which could be 100s of characters overflowing your buffers == bad
  if ((value > PRINTF_MAX_FLOAT) || (value < -PRINTF_MAX_FLOAT)) {
#if defined(PRINTF_SUPPORT_EXPONENTIAL)
    return _etoa(out, buffer, idx, maxlen, value, prec, width, flags);
#else
    return 0U;
#endif
  }

  // test for negative
  bool negative = false;
  if (value < 0) {
    negative = true;
    value = 0 - value;
  }

  // set default precision, if not set explicitly
  if (!(flags & FLAGS_PRECISION)) {
    prec = PRINTF_DEFAULT_FLOAT_PRECISION;
  }
  // limit precision to 9, cause a prec >= 10 can lead to overflow errors
  while ((len < PRINTF_FTOA_BUFFER_SIZE) && (prec > 9U)) {
    buf[len++] = '0';
    prec--;
  }

  int whole = (int)value;
  double tmp = (value - whole) * pow10[prec];
  unsigned long frac = (unsigned long)tmp;
  diff = tmp - frac;

  if (diff > 0.5) {
    ++frac;
    // handle rollover, e.g. case 0.99 with prec 1 is 1.0
    if (frac >= pow10[prec]) {
      frac = 0;
      ++whole;
    }
  }
  else if (diff < 0.5) {
  }
  else if ((frac == 0U) || (frac & 1U)) {
    // if halfway, round up if odd OR if last digit is 0
    ++frac;
  }

  if (prec == 0U) {
    diff = value - (double)whole;
    if ((!(diff < 0.5) || (diff > 0.5)) && (whole & 1)) {
      // exactly 0.5 and ODD, then round up
      // 1.5 -> 2, but 2.5 -> 2
      ++whole;
    }
  }
  else {
    unsigned int count = prec;
    // now do fractional part, as an unsigned number
    while (len < PRINTF_FTOA_BUFFER_SIZE) {
      --count;
      buf[len++] = (char)(48U + (frac % 10U));
      if (!(frac /= 10U)) {
        break;
      }
    }
    // add extra 0s
    while ((len < PRINTF_FTOA_BUFFER_SIZE) && (count-- > 0U)) {
      buf[len++] = '0';
    }
    if (len < PRINTF_FTOA_BUFFER_SIZE) {
      // add decimal
      buf[len++] = '.';
    }
  }

  // do whole part, number is reversed
  while (len < PRINTF_FTOA_BUFFER_SIZE) {
    buf[len++] = (char)(48 + (whole % 10));
    if (!(whole /= 10)) {
      break;
    }
  }

  // pad leading zeros
  if (!(flags & FLAGS_LEFT) && (flags & FLAGS_ZEROPAD)) {
    if (width && (negative || (flags & (FLAGS_PLUS | FLAGS_SPACE)))) {
      width--;
    }
    while ((len < width) && (len < PRINTF_FTOA_BUFFER_SIZE)) {
      buf[len++] = '0';
    }
  }

  if (len < PRINTF_FTOA_BUFFER_SIZE) {
    if (negative) {
      buf[len++] = '-';
    }
    else if (flags & FLAGS_PLUS) {
      buf[len++] = '+';  // ignore the space if the '+' exists
    }
    else if (flags & FLAGS_SPACE) {
      buf[len++] = ' ';
    }
  }

  return _out_rev(out, buffer, idx, maxlen, buf, len, width, flags);
}


#if defined(PRINTF_SUPPORT_EXPONENTIAL)
// internal ftoa variant for exponential floating-point type, contributed by Martijn Jasperse <m.jasperse@gmail.com>
static size_t _etoa(out_fct_type out, char* buffer, size_t idx, size_t maxlen, double value, unsigned int prec, unsigned int width, unsigned int flags)
{
  // check for NaN and special values
  if ((value != value) || (value > DBL_MAX) || (value < -DBL_MAX)) {
    return _ftoa(out, buffer, idx, maxlen, value, prec, width, flags);
  }

  // determine the sign
  const bool negative = value < 0;
  if (negative) {
    value = -value;
  }

  // default precision
  if (!(flags & FLAGS_PRECISION)) {
    prec = PRINTF_DEFAULT_FLOAT_PRECISION;
  }

  // determine the decimal exponent
  // based on the algorithm by David Gay (https://www.ampl.com/netlib/fp/dtoa.c)
  union {
    uint64_t U;
    double   F;
  } conv;

  conv.F = value;
  int exp2 = (int)((conv.U >> 52U) & 0x07FFU) - 1023;           // effectively log2
  conv.U = (conv.U & ((1ULL << 52U) - 1U)) | (1023ULL << 52U);  // drop the exponent so conv.F is now in [1,2)
  // now approximate log10 from the log2 integer part and an expansion of ln around 1.5
  int expval = (int)(0.1760912590558 + exp2 * 0.301029995663981 + (conv.F - 1.5) * 0.289529654602168);
  // now we want to compute 10^expval but we want to be sure it won't overflow
  exp2 = (int)(expval * 3.321928094887362 + 0.5);
  const double z  = expval * 2.302585092994046 - exp2 * 0.6931471805599453;
  const double z2 = z * z;
  conv.U = (uint64_t)(exp2 + 1023) << 52U;
  // compute exp(z) using continued fractions, see https://en.wikipedia.org/wiki/Exponential_function#Continued_fractions_for_ex
  conv.F *= 1 + 2 * z / (2 - z + (z2 / (6 + (z2 / (10 + z2 / 14)))));
  // correct for rounding errors
  if (value < conv.F) {
    expval--;
    conv.F /= 10;
  }

  // the exponent format is "%+03d" and largest value is "307", so set aside 4-5 characters
  unsigned int minwidth = ((expval < 100) && (expval > -100)) ? 4U : 5U;

  // in "%g" mode, "prec" is the number of *significant figures* not decimals
  if (flags & FLAGS_ADAPT_EXP) {
    // do we want to fall-back to "%f" mode?
    if ((value >= 1e-4) && (value < 1e6)) {
      if ((int)prec > expval) {
        prec = (unsigned)((int)prec - expval - 1);
      }
      else {
        prec = 0;
      }
      flags |= FLAGS_PRECISION;   // make sure _ftoa respects precision
      // no characters in exponent
      minwidth = 0U;
      expval   = 0;
    }
    else {
      // we use one sigfig for the whole part
      if ((prec > 0) && (flags & FLAGS_PRECISION)) {
        --prec;
      }
    }
  }

  // will everything fit?
  unsigned int fwidth = width;
  if (width > minwidth) {
    // we didn't fall-back so subtract the characters required for the exponent
    fwidth -= minwidth;
  } else {
    // not enough characters, so go back to default sizing
    fwidth = 0U;
  }
  if ((flags & FLAGS_LEFT) && minwidth) {
    // if we're padding on the right, DON'T pad the floating part
    fwidth = 0U;
  }

  // rescale the float value
  if (expval) {
    value /= conv.F;
  }

  // output the floating part
  const size_t start_idx = idx;
  idx = _ftoa(out, buffer, idx, maxlen, negative ? -value : value, prec, fwidth, flags & ~FLAGS_ADAPT_EXP);

  // output the exponent part
  if (!prec && minwidth) {
    // output the exponential symbol
    out((flags & FLAGS_UPPERCASE) ? 'E' : 'e', buffer, idx++, maxlen);
    // output the exponent value
    idx = _ntoa_long(out, buffer, idx, maxlen, (expval < 0) ? -expval : expval, expval < 0, 10, 0, minwidth-1, FLAGS_ZEROPAD | FLAGS_PLUS);
    // might need to right-pad spaces
    if (flags & FLAGS_LEFT) {
      while (idx - start_idx < width) out(' ', buffer, idx++, maxlen);
    }
  }
  return idx;
}
#endif  // PRINTF_SUPPORT_EXPONENTIAL
#endif  // PRINTF_SUPPORT_FLOAT


// internal vsnprintf
static int _vsnprintf(out_fct_type out, char* buffer, const size_t maxlen, const char* format, va_list va)
{
  unsigned int flags, width, precision, n;
  size_t idx = 0U;

  if (!buffer) {
    // use null output function
    out = _out_null;
  }

  while (*format)
  {
    // format specifier?  %[flags][width][.precision][length]
    if (*format != '%') {
      // no
      out(*format, buffer, idx++, maxlen);
      format++;
      continue;
    }
    else {
      // yes, evaluate it
      format++;
    }

    // evaluate flags
    flags = 0U;
    do {
      switch (*format) {
        case '0': flags |= FLAGS_ZEROPAD; format++; n = 1U; break;
        case '-': flags |= FLAGS_LEFT;    format++; n = 1U; break;
        case '+': flags |= FLAGS_PLUS;    format++; n = 1U; break;
        case ' ': flags |= FLAGS_SPACE;   format++; n = 1U; break;
        case '#': flags |= FLAGS_HASH;    format++; n = 1U; break;
        default :                                   n = 0U; break;
      }
    } while (n);

    // evaluate width field
    width = 0U;
    if (_is_digit(*format)) {
      width = _atoi(&format);
    }
    else if (*format == '*') {
      const int w = va_arg(va, int);
      if (w < 0) {
        flags |= FLAGS_LEFT;    // reverse padding
        width = (unsigned int)-w;
      }
      else {
        width = (unsigned int)w;
      }
      format++;
    }

    // evaluate precision field
    precision = 0U;
    if (*format == '.') {
      flags |= FLAGS_PRECISION;
      format++;
      if (_is_digit(*format)) {
        precision = _atoi(&format);
      }
      else if (*format == '*') {
        const int prec = (int)va_arg(va, int);
        precision = prec > 0 ? (unsigned int)prec : 0U;
        format++;
      }
    }

    // evaluate length field
    switch (*format) {
      case 'l' :
        flags |= FLAGS_LONG;
        format++;
        if (*format == 'l') {
          flags |= FLAGS_LONG_LONG;
          format++;
        }
        break;
      case 'h' :
        flags |= FLAGS_SHORT;
        format++;
        if (*format == 'h') {
          flags |= FLAGS_CHAR;
          format++;
        }
        break;
#if defined(PRINTF_SUPPORT_PTRDIFF_T)
      case 't' :
        flags |= (sizeof(ptrdiff_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
        format++;
        break;
#endif
      case 'j' :
        flags |= (sizeof(intmax_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
        format++;
        break;
      case 'z' :
        flags |= (sizeof(size_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
        format++;
        break;
      default :
        break;
    }

    // evaluate specifier
    switch (*format) {
      case 'd' :
      case 'i' :
      case 'u' :
      case 'x' :
      case 'X' :
      case 'o' :
      case 'b' : {
        // set the base
        unsigned int base;
        if (*format == 'x' || *format == 'X') {
          base = 16U;
        }
        else if (*format == 'o') {
          base =  8U;
        }
        else if (*format == 'b') {
          base =  2U;
        }
        else {
          base = 10U;
          flags &= ~FLAGS_HASH;   // no hash for dec format
        }
        // uppercase
        if (*format == 'X') {
          flags |= FLAGS_UPPERCASE;
        }

        // no plus or space flag for u, x, X, o, b
        if ((*format != 'i') && (*format != 'd')) {
          flags &= ~(FLAGS_PLUS | FLAGS_SPACE);
        }

        // ignore '0' flag when precision is given
        if (flags & FLAGS_PRECISION) {
          flags &= ~FLAGS_ZEROPAD;
        }

        // convert the integer
        if ((*format == 'i') || (*format == 'd')) {
          // signed
          if (flags & FLAGS_LONG_LONG) {
#if defined(PRINTF_SUPPORT_LONG_LONG)
            const long long value = va_arg(va, long long);
            idx = _ntoa_long_long(out, buffer, idx, maxlen, (unsigned long long)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
#endif
          }
          else if (flags & FLAGS_LONG) {
            const long value = va_arg(va, long);
            idx = _ntoa_long(out, buffer, idx, maxlen, (unsigned long)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
          }
          else {
            const int value = (flags & FLAGS_CHAR) ? (char)va_arg(va, int) : (flags & FLAGS_SHORT) ? (short int)va_arg(va, int) : va_arg(va, int);
            idx = _ntoa_long(out, buffer, idx, maxlen, (unsigned int)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
          }
        }
        else {
          // unsigned
          if (flags & FLAGS_LONG_LONG) {
#if defined(PRINTF_SUPPORT_LONG_LONG)
            idx = _ntoa_long_long(out, buffer, idx, maxlen, va_arg(va, unsigned long long), false, base, precision, width, flags);
#endif
          }
          else if (flags & FLAGS_LONG) {
            idx = _ntoa_long(out, buffer, idx, maxlen, va_arg(va, unsigned long), false, base, precision, width, flags);
          }
          else {
            const unsigned int value = (flags & FLAGS_CHAR) ? (unsigned char)va_arg(va, unsigned int) : (flags & FLAGS_SHORT) ? (unsigned short int)va_arg(va, unsigned int) : va_arg(va, unsigned int);
            idx = _ntoa_long(out, buffer, idx, maxlen, value, false, base, precision, width, flags);
          }
        }
        format++;
        break;
      }
#if defined(PRINTF_SUPPORT_FLOAT)
      case 'f' :
      case 'F' :
        if (*format == 'F') flags |= FLAGS_UPPERCASE;
        idx = _ftoa(out, buffer, idx, maxlen, va_arg(va, double), precision, width, flags);
        format++;
        break;
#if defined(PRINTF_SUPPORT_EXPONENTIAL)
      case 'e':
      case 'E':
      case 'g':
      case 'G':
        if ((*format == 'g')||(*format == 'G')) flags |= FLAGS_ADAPT_EXP;
        if ((*format == 'E')||(*format == 'G')) flags |= FLAGS_UPPERCASE;
        idx = _etoa(out, buffer, idx, maxlen, va_arg(va, double), precision, width, flags);
        format++;
        break;
#endif  // PRINTF_SUPPORT_EXPONENTIAL
#endif  // PRINTF_SUPPORT_FLOAT
      case 'c' : {
        unsigned int l = 1U;
        // pre padding
        if (!(flags & FLAGS_LEFT)) {
          while (l++ < width) {
            out(' ', buffer, idx++, maxlen);
          }
        }
        // char output
        out((char)va_arg(va, int), buffer, idx++, maxlen);
        // post padding
        if (flags & FLAGS_LEFT) {
          while (l++ < width) {
            out(' ', buffer, idx++, maxlen);
          }
        }
        format++;
        break;
      }

      case 's' : {
        const char* p = va_arg(va, char*);
        unsigned int l = _strnlen_s(p, precision ? precision : (size_t)-1);
        // pre padding
        if (flags & FLAGS_PRECISION) {
          l = (l < precision ? l : precision);
        }
        if (!(flags & FLAGS_LEFT)) {
          while (l++ < width) {
            out(' ', buffer, idx++, maxlen);
          }
        }
        // string output
        while ((*p != 0) && (!(flags & FLAGS_PRECISION) || precision--)) {
          out(*(p++), buffer, idx++, maxlen);
        }
        // post padding
        if (flags & FLAGS_LEFT) {
          while (l++ < width) {
            out(' ', buffer, idx++, maxlen);
          }
        }
        format++;
        break;
      }

      case 'p' : {
        width = sizeof(void*) * 2U;
        flags |= FLAGS_ZEROPAD | FLAGS_UPPERCASE;
#if defined(PRINTF_SUPPORT_LONG_LONG)
        const bool is_ll = sizeof(uintptr_t) == sizeof(long long);
        if (is_ll) {
          idx = _ntoa_long_long(out, buffer, idx, maxlen, (uintptr_t)va_arg(va, void*), false, 16U, precision, width, flags);
        }
        else {
#endif
          idx = _ntoa_long(out, buffer, idx, maxlen, (unsigned long)((uintptr_t)va_arg(va, void*)), false, 16U, precision, width, flags);
#if defined(PRINTF_SUPPORT_LONG_LONG)
        }
#endif
        format++;
        break;
      }

      case 'M' : {
        const char* p = va_arg(va, char*);
        char store[40];
        unsigned int l = __mac2str((unsigned char *)p, store);
        const char* pstr = &store[0];
        // pre padding
        if (flags & FLAGS_PRECISION) {
          l = (l < precision ? l : precision);
        }
        if (!(flags & FLAGS_LEFT)) {
          while (l++ < width) {
            out(' ', buffer, idx++, maxlen);
          }
        }
        // string output
        while ((*pstr != 0) && (!(flags & FLAGS_PRECISION) || precision--)) {
          out(*(pstr++), buffer, idx++, maxlen);
        }
        // post padding
        if (flags & FLAGS_LEFT) {
          while (l++ < width) {
            out(' ', buffer, idx++, maxlen);
          }
        }
        format++;
        break;
      }
    case 'v' : {
        uint32_t ipv4 = va_arg(va, uint32_t);
        char store[40];
        unsigned int l = __ip2str(4, &ipv4, store);
        const char* pstr = &store[0];
        // pre padding
        if (flags & FLAGS_PRECISION) {
          l = (l < precision ? l : precision);
        }
        if (!(flags & FLAGS_LEFT)) {
          while (l++ < width) {
            out(' ', buffer, idx++, maxlen);
          }
        }
        // string output
        while ((*pstr != 0) && (!(flags & FLAGS_PRECISION) || precision--)) {
          out(*(pstr++), buffer, idx++, maxlen);
        }
        // post padding
        if (flags & FLAGS_LEFT) {
          while (l++ < width) {
            out(' ', buffer, idx++, maxlen);
          }
        }
        format++;
        break;
      }
      case 'V' : {
        char *ipv6 = va_arg(va, char*);
        char store[40];
        unsigned int l = __ip2str(6, (unsigned int *)ipv6, store);
        const char* pstr = &store[0];
        // pre padding
        if (flags & FLAGS_PRECISION) {
          l = (l < precision ? l : precision);
        }
        if (!(flags & FLAGS_LEFT)) {
          while (l++ < width) {
            out(' ', buffer, idx++, maxlen);
          }
        }
        // string output
        while ((*pstr != 0) && (!(flags & FLAGS_PRECISION) || precision--)) {
          out(*(pstr++), buffer, idx++, maxlen);
        }
        // post padding
        if (flags & FLAGS_LEFT) {
          while (l++ < width) {
            out(' ', buffer, idx++, maxlen);
          }
        }
        format++;
        break;
      }

      case '%' :
        out('%', buffer, idx++, maxlen);
        format++;
        break;

      default :
        out(*format, buffer, idx++, maxlen);
        format++;
        break;
    }
  }

  // termination
  out((char)0, buffer, idx < maxlen ? idx : maxlen - 1U, maxlen);

  // return written chars without terminating \0
  return (int)idx;
}

int wm_vsnprintf(char* buffer, size_t count, const char* format, va_list va)
{
  return _vsnprintf(_out_buffer, buffer, count, format, va);
}


int wm_printf(const char *fmt,...)
{
    va_list args;
    size_t length;

	va_start(args, fmt);
	length = _vsnprintf(_out_uart, (char*)fmt, (size_t) - 1, fmt, args);
	va_end(args);

	return length;
}

int wm_vprintf(const char *fmt, va_list arg_ptr)
{
    size_t length;

	length = _vsnprintf(_out_uart, (char*)fmt, (size_t) - 1, fmt, arg_ptr);

	return length;
}

#if 1//defined(_NEWLIB_VERSION_H__)
 __attribute__((weak)) int sscanf(const char *str, const char *format, ...) /* bug: replace 3.10.21 newlib */
{
    va_list args;
    int i;

    va_start(args,format);
    i = wm_vsscanf(str, format, args);
    va_end(args);

    return i;
}

__attribute__((weak)) int __cskyvscanfsscanf(const char *str, const char *format, ...)
{
    va_list args;
	int i;

	va_start(args,format);
	i = wm_vsscanf(str, format, args);
	va_end(args);
	
	return i;
}

__attribute__((weak)) int __cskyvprintfsprintf(char *str, const char *format, ...)
{
    va_list ap;
    int i;

	va_start(ap, format);
	i = wm_vsnprintf(str, (size_t) - 1, format, ap);
	va_end(ap);

	return i;
}

__attribute__((weak)) int __cskyvprintfsnprintf(char *str, size_t size, const char *format, ...)
{
    va_list ap;
    int i;

	va_start(ap, format);
	i = wm_vsnprintf(str, size, format, ap);
	va_end(ap);

	return i;
}

__attribute__((weak)) int __cskyvprintfvsnprintf(char *str, size_t size, const char *format, va_list ap)
{
    return wm_vsnprintf(str, size, format, ap);
}

__attribute__((weak)) int __cskyvprintfvsprintf(char *str, const char *format, va_list ap)
{
    return wm_vsnprintf(str, (size_t) - 1, format, ap);
}

int __cskyvprintfprintf(const char *fmt, ...) __attribute__((weak, alias("wm_printf")));

__attribute__((weak)) void __assert_fail(const char *file,
	int line,
	const char *func,
	const char *failedexpr)
{
    wm_printf("assertion \"%s\" failed: file \"%s\", line %d%s%s\r\n",
	   failedexpr, file, line,
	   func ? ", function: " : "", func ? func : "");
    while(1);
}

/*For CPP type used, you first call this function in main entry*/
extern int __dtor_end__;
extern int __ctor_end__;
extern int __ctor_start__;
typedef void (*func_ptr)(void);
__attribute__((weak)) void cxx_system_init(void)
{
    func_ptr *p;
    for (p = (func_ptr *)&__ctor_end__ -1; p >= (func_ptr *)&__ctor_start__; p--)
    {
        (*p)();
    }
}
#endif

