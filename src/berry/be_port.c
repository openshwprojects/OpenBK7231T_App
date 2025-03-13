/********************************************************************
** Copyright (c) 2018-2020 Guan Wenliang
** This file is part of the Berry default interpreter.
** skiars@qq.com, https://github.com/Skiars/berry
** See Copyright Notice in the LICENSE file or at
** https://github.com/Skiars/berry/blob/master/LICENSE
********************************************************************/

#include "berry.h"
#include "be_mem.h"
#include "be_sys.h"
#include <stdio.h>
#include <string.h>

#include "../logging/logging.h"
#include "../littlefs/our_lfs.h"

/* this file contains configuration for the file system. */

/* standard input and output */

BERRY_API void be_writebuffer(const char* buffer, size_t length)
{
	ADDLOG_INFO(LOG_FEATURE_BERRY, "%.*s", length, buffer);
}

BERRY_API char* be_readstring(char* buffer, size_t size)
{
	if((size > 0) && (buffer != NULL))
	{
		*buffer = 0;
	}
	return buffer;
}

#if ENABLE_LITTLEFS


// Mapping of fopen modes to lfs_file_open flags
static int mode_to_flags(const char* mode)
{
	// accepted regex: ^(r|w|a)b?+?.*$
	bool has_plus = false;
	if(*mode)
	{
		// skip r, w, b
		const char* extra = mode + 1;
		// skip allowed b
		if(*extra == 'b') extra++;
		has_plus = *extra == '+';
	}

	switch(mode[0])
	{
		case 'r':
			return has_plus ? LFS_O_RDWR : LFS_O_RDONLY;
		case 'w':
			return has_plus ?
				LFS_O_RDWR | LFS_O_CREAT | LFS_O_TRUNC :
				LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC;
		case 'a':
			return has_plus ?
				LFS_O_RDWR | LFS_O_CREAT | LFS_O_APPEND :
				LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND;
	}
	return -1;
}

void* be_fopen(const char* filename, const char* modes)
{
	if(!lfs_present()) init_lfs(1);
	if(lfs_present())
	{
		int flags = mode_to_flags(modes);
		if(flags == -1)
		{
			return NULL;
		}
		lfs_file_t* file = malloc(sizeof(lfs_file_t));
		memset(file, 0, sizeof(lfs_file_t));
		int err = lfs_file_open(&lfs, file, filename, flags);
		if(err)
		{
			free(file);
			return NULL;
		}
		return file;
	}
	return NULL;
}

int be_fclose(void* hfile)
{
	if(hfile != NULL)
	{
		int ret = lfs_file_close(&lfs, (lfs_file_t*)hfile);
		free(hfile);
		return ret;
	}
	return -1;
}

size_t be_fwrite(void* hfile, const void* buffer, size_t length)
{
	if(hfile != NULL) return lfs_file_write(&lfs, hfile, buffer, length);
	return 0;
}

size_t be_fread(void* hfile, void* buffer, size_t length)
{
	if(hfile != NULL) return lfs_file_read(&lfs, hfile, buffer, length);
	return 0;
}

char* be_fgets(void* hfile, void* buffer, int size)
{
	if(hfile != NULL)
	{
		if(size < 1) return NULL;

		char* dest = buffer;
		int count = 0;

		while(count < size - 1)
		{
			if(lfs_file_read(&lfs, hfile, dest, 1) != 1)
			{
				// EOF or error
				if(count == 0) return NULL;
				break;
			}

			// Check for newline
			if(*dest == '\n')
			{
				count++;
				dest++;
				break;
			}

			count++;
			dest++;
		}

		// Null-terminate the string
		*dest = '\0';
		return buffer;
	}
	return NULL;
}

int be_fseek(void* hfile, long offset)
{
	if(hfile != NULL) return lfs_file_seek(&lfs, hfile, offset, LFS_SEEK_SET);
	return -1;
}

long int be_ftell(void* hfile)
{
	if(hfile != NULL) return lfs_file_tell(&lfs, hfile);
	return 0;
}

long int be_fflush(void* hfile)
{
	if(hfile != NULL) return lfs_file_sync(&lfs, hfile);
	return 0;
}

size_t be_fsize(void* hfile)
{
	if(hfile != NULL) return lfs_file_size(&lfs, hfile);
	return 0;
}

#else // !ENABLE_LITTLEFS

void* be_fopen(const char* filename, const char* modes)
{
	return NULL;
}

int be_fclose(void* hfile)
{
	return -1;
}

size_t be_fwrite(void* hfile, const void* buffer, size_t length)
{
	return 0;
}

size_t be_fread(void* hfile, void* buffer, size_t length)
{
	return 0;
}

char* be_fgets(void* hfile, void* buffer, int size)
{
	return NULL;
}

int be_fseek(void* hfile, long offset)
{
	return -1;
}

long int be_ftell(void* hfile)
{
	return 0;
}

long int be_fflush(void* hfile)
{
	return 0;
}

size_t be_fsize(void* hfile)
{
	return 0;
}

#endif // ENABLE_LITTLEFS
