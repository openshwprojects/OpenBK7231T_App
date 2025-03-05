/********************************************************************
** Copyright (c) 2018-2020 Guan Wenliang
** This file is part of the Berry default interpreter.
** skiars@qq.com, https://github.com/Skiars/berry
** See Copyright Notice in the LICENSE file or at
** https://github.com/Skiars/berry/blob/master/LICENSE
********************************************************************/
#include "../logging/logging.h"

#include "berry.h"
#include "be_mem.h"
#include "be_sys.h"
#include <stdio.h>
#include <string.h>

/* this file contains configuration for the file system. */

/* standard input and output */

BERRY_API void be_writebuffer(const char *buffer, size_t length)
{
	ADDLOG_INFO(LOG_FEATURE_BERRY, "%.*s", length, buffer);
}

BERRY_API char* be_readstring(char *buffer, size_t size)
{
  if ((size > 0) && (buffer != NULL)) {
    *buffer = 0;
  }
  return buffer;
}

void* be_fopen(const char *filename, const char *modes)
{
  return NULL;
}

int be_fclose(void *hfile)
{
    return -1;
}

size_t be_fwrite(void *hfile, const void *buffer, size_t length)
{
    return 0;
}

size_t be_fread(void *hfile, void *buffer, size_t length)
{
    return 0;
}

char* be_fgets(void *hfile, void *buffer, int size)
{
    return NULL;
}

int be_fseek(void *hfile, long offset)
{
    return -1;
}

long int be_ftell(void *hfile)
{
    return 0;
}

long int be_fflush(void *hfile)
{
    return 0;
}

size_t be_fsize(void *hfile)
{
    return 0;
}
