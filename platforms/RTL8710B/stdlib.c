// copied from https://github.com/libretiny-eu/libretiny/blob/master/cores/realtek-ambz/base/wraps/stdlib.c
#include <stddef.h>

#define ROM __attribute__((long_call))

ROM int prvAtoi(const char* str);
ROM long simple_strtol(const char* str, char** str_end, int base);
ROM unsigned long simple_strtoul(const char* str, char** str_end, int base);
ROM int Rand();
ROM char* __rtl_strcat_v1_00(char* dest, const char* src);
ROM char* _strcpy(char* dest, const char* src);
ROM char* __rtl_strncat_v1_00(char* dest, const char* src, size_t count);
ROM char* _strncpy(char* dest, const char* src, size_t count);
ROM char* _strchr(const char* str, int ch);
ROM int prvStrCmp(const char* lhs, const char* rhs);
ROM size_t prvStrLen(const char* str);
ROM int _strncmp(const char* lhs, const char* rhs, size_t count);
ROM char* _strpbrk(const char* dest, const char* breakset);
ROM char* prvStrStr(const char* str, const char* substr);
ROM char* prvStrtok(char* str, const char* delim);
ROM void* __rtl_memchr_v1_00(const void* ptr, int ch, size_t count);
ROM int _memcmp(const void* lhs, const void* rhs, size_t count);
ROM void* _memcpy(void* dest, const void* src, size_t count);
ROM void* __rtl_memmove_v1_00(void* dest, const void* src, size_t count);
ROM void* _memset(void* dest, int ch, size_t count);
ROM char* _strsep(char** stringp, const char* delim);

int __wrap_atoi(const char* str)
{
	register int num, neg;
	register char c;
	num = neg = 0;
	c = *str;
	while((c == ' ') || (c == '\n') || (c == '\r') || (c == '\t'))c = *++str;
	if(c == '-')
	{ /* get an optional sign */
		neg = 1;
		c = *++str;
	}
	else if(c == '+')
	{
		c = *++str;
	}

	while((c >= '0') && (c <= '9'))
	{
		num = (10 * num) + (c - '0');
		c = *++str;
	}
	if(neg)return(0 - num);
	return(num);
}

long __wrap_atol(const char* str)
{
	return simple_strtol(str, NULL, 10);
}

long __wrap_strtol(const char* str, char** str_end, int base)
{
	return simple_strtol(str, str_end, base);
}

unsigned long __wrap_strtoul(const char* str, char** str_end, int base)
{
	return simple_strtoul(str, str_end, base);
}

int __wrap_rand()
{
	return Rand();
}

char* __wrap_strcat(char* dest, const char* src)
{
	return __rtl_strcat_v1_00(dest, src);
}

char* __wrap_strcpy(char* dest, const char* src)
{
	return _strcpy(dest, src);
}

char* __wrap_strncat(char* dest, const char* src, size_t count)
{
	return __rtl_strncat_v1_00(dest, src, count);
}

char* __wrap_strncpy(char* dest, const char* src, size_t count)
{
	return _strncpy(dest, src, count);
}

char* __wrap_strchr(const char* str, int ch)
{
	return _strchr(str, ch);
}

int __wrap_strcmp(const char* lhs, const char* rhs)
{
	return prvStrCmp(lhs, rhs);
}

size_t __wrap_strlen(const char* str)
{
	return prvStrLen(str);
}

int __wrap_strncmp(const char* lhs, const char* rhs, size_t count)
{
	return _strncmp(lhs, rhs, count);
}

char* __wrap_strpbrk(const char* dest, const char* breakset)
{
	return _strpbrk(dest, breakset);
}

char* __wrap_strstr(const char* str, const char* substr)
{
	return prvStrStr(str, substr);
}

char* __wrap_strtok(char* str, const char* delim)
{
	return prvStrtok(str, delim);
}

void* __wrap_memchr(const void* ptr, int ch, size_t count)
{
	return __rtl_memchr_v1_00(ptr, ch, count);
}

int __wrap_memcmp(const void* lhs, const void* rhs, size_t count)
{
	return _memcmp(lhs, rhs, count);
}

void* __wrap_memcpy(void* dest, const void* src, size_t count)
{
	return _memcpy(dest, src, count);
}

void* __wrap_memmove(void* dest, const void* src, size_t count)
{
	return __rtl_memmove_v1_00(dest, src, count);
}

void* __wrap_memset(void* dest, int ch, size_t count)
{
	return _memset(dest, ch, count);
}

char* __wrap_strsep(char** stringp, const char* delim)
{
	return _strsep(stringp, delim);
}
