/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//#include "typedef.h"
#include "os.h"
//#include "hal.h"
//#include "drv.h"
//#include "parm.h"
//#include "sys_def.h"
//#include "net.h"

#include "fs.h"
#include "fsdata.h"
#include "wm_flash.h"
#include "lwip/memp.h"
#include "wm_wifi_oneshot.h"

#include "httpd.h"

#define FSDATA_BASE_ADDR            (0xd0000)
#define NDEBUG 1
#ifndef NDEBUG
#define FSDATA_IN_EXT_FLASH
#endif

#ifdef FSDATA_IN_EXT_FLASH
#define FS_ROOT  (FSDATA_BASE_ADDR+4)
#else

#if TLS_CONFIG_AP_MODE_ONESHOT
#include "fsdata_ap_config.c"
#else
#include "fsdata.c"
#endif

#endif
extern u16  Web_parse_line(char * id_char,u16 * after_id_len,char * idvalue,u16 * Value_Offset,u8 * Id_type);

/*-----------------------------------------------------------------------------------*/
/* Define the number of open files that we can support. */
#ifndef LWIP_MAX_OPEN_FILES
#define LWIP_MAX_OPEN_FILES     10
#endif

/* Define the file system memory allocation structure. */
struct fs_table {
  struct fs_file file;
};

/* Allocate file system memory */
struct fs_table *fs_memory[LWIP_MAX_OPEN_FILES];
#if WEB_SERVER_RUSSIAN
INT gCurHtmlFile = 0;		// 1: russian html 0: english
#endif
/*-----------------------------------------------------------------------------------*/
static struct fs_file *fs_malloc(void)
{
	int i;
	for(i = 0; i < LWIP_MAX_OPEN_FILES; i++) 
	{
		if(NULL == fs_memory[i]) 
		{
			fs_memory[i] = tls_mem_alloc(sizeof(struct fs_table));
			if (fs_memory[i])
			{
				return(&fs_memory[i]->file);
			}
			else
			{
				return NULL;
			}
		}
	}
	return(NULL);
}

/*-----------------------------------------------------------------------------------*/
static void fs_free(struct fs_file *file)
{
	int i;
	for(i = 0; i < LWIP_MAX_OPEN_FILES; i++) 
	{
		if(fs_memory[i] && (&fs_memory[i]->file == file))
		{
			tls_mem_free(fs_memory[i]);
			fs_memory[i] = NULL;
			break;
		}
  }
  return;
}
/*-----------------------------------------------------------------------------------*/
#ifdef FSDATA_IN_EXT_FLASH
/* advance.html has max line of 530 bytes !!! */
#define FS_MAX_READ_LINE 1024

char *ext_flash_read_line(unsigned int buffer_ptr)
{
    static char buffer[FS_MAX_READ_LINE*2];
    static unsigned int buffer_base = 0;
    static int init = 0;

	if ((!init)||(buffer_ptr < buffer_base)||(buffer_ptr > (buffer_base+FS_MAX_READ_LINE)))
	{
		init = 1;
		buffer_base = buffer_ptr;
		tls_fls_read(buffer_base, (u8 *)buffer, FS_MAX_READ_LINE*2);
	}
	return &buffer[buffer_ptr -buffer_base];
}

unsigned int ext_flash_read_int32(unsigned int buffer_ptr)
{
    #define BUFFER  128
    static u8 flash_buffer[BUFFER];
    static unsigned int buffer_base = 0;
    static int init = 0;
    unsigned int r;	

    if ((!init)||(buffer_ptr < buffer_base)||((buffer_ptr+4) > (buffer_base+BUFFER))){
        buffer_base = buffer_ptr - (BUFFER/2);
        tls_fls_read(buffer_base, flash_buffer, BUFFER);
    }

    buffer_ptr -= buffer_base;

    r = flash_buffer[buffer_ptr];
    r += (flash_buffer[buffer_ptr+1]<<8) ;
    r += (flash_buffer[buffer_ptr+2]<<16) ;
    r += (flash_buffer[buffer_ptr+3]<<24) ;

#undef BUFFER
    return r;
}

struct fs_file *fs_open(char *name)
{
  struct fs_file *file;
  const struct fsdata_file *f;
  char *fsdata;
  unsigned int fname_addr;

  file = fs_malloc();
  if(file == NULL) {
    return NULL;
  }

  for(f = (struct fsdata_file *)ext_flash_read_int32((unsigned int)FS_ROOT); f != NULL; \
  	f = (struct fsdata_file *)ext_flash_read_int32((unsigned int)(&f->next))) 
  {
      fname_addr = ext_flash_read_int32((unsigned int)(&f->name));
      fsdata = ext_flash_read_line(fname_addr);	  

    if (!strcmp(name, fsdata)) {
      file->data = (char *)ext_flash_read_int32((unsigned int)(&f->data));
      file->len = ext_flash_read_int32((unsigned int)(&f->len));
      file->index = file->len;
      file->pextension = NULL;
      file->ReadIndex=0;
      return file;
    }
  }
  
  fs_free(file);
  return NULL;
}
#else
struct fs_file *fs_open(char *name)
{
	struct fs_file *file;
	const struct fsdata_file *f;
  
	DEBUG_PRINT("kevin debug fs_open = %s\n\r", name);
	file = fs_malloc();
	if(file == NULL) 
	{
		return NULL;
	}
#if WEB_SERVER_RUSSIAN  
	if(!strcmp(name, "/basic_ru.html"))
	{
		gCurHtmlFile = 1;
	}
	else if(!strcmp(name, "/basic_en.html"))
	{
		gCurHtmlFile = 0;
	}
#endif  
	for(f = FS_ROOT; f != NULL; f = f->next) 
	{
		if (!strcmp(name, (char *)f->name)) 
		{
			file->data = (char *)f->data;
			file->len = f->len;
			file->index = f->len;
			file->pextension = NULL;
			file->ReadIndex=0;
			return file;
		}
	}
	fs_free(file);
	return NULL;
}
#endif

/*-----------------------------------------------------------------------------------*/
void fs_close(struct fs_file *file)
{
	fs_free(file);
}
/*-----------------------------------------------------------------------------------*/
int fs_read(struct fs_file *file, char *buffer, int count)
{
	int read;

	if(file->index == file->len) 
	{
		return -1;
	}
	read = file->len - file->index;
	if(read > count)
	{
		read = count;
	}
	memcpy(buffer, (file->data + file->index), read);
	file->index += read;
	return(read);
}

/*-----------------------------------------------------------------------------------
// Description:   Read one line text into buffer s, and return its length 
// Parameters:  file:
			   char buffer :buffer to store the text read from file 
//                     max_length: max length of one line  
*/
int fs_read_line(const void *ori_data, char  *buffer, int  max_length, int *tembuflen)
{
	#define MAX_ID_VALUE_BUFFER_LEN  2048
	int len=0; 
	char * temp, *data = NULL;
	char * idvaluebuffer=NULL;///[MAX_ID_BUFFER_LEN];
	u16 beforeidlen=0;
	u16 Idlen=0;
	u16 startatfteid=0;
	u16  Lenchange=0;
	u8  IdType=0;

       #ifdef FSDATA_IN_EXT_FLASH
       data = ext_flash_read_line((unsigned int)ori_data);
       #else
       data = (char *)ori_data;
       #endif	   

	if(*tembuflen >= 1024)
	{
		return -1;
	}
	idvaluebuffer = mem_malloc(MAX_ID_VALUE_BUFFER_LEN);
	if(idvaluebuffer == NULL)
	{
		return -1;
	}
	memset(idvaluebuffer, 0, MAX_ID_VALUE_BUFFER_LEN);

	temp = (char *)data;
	while(*temp++ != '\n')
	{
		if(*temp == 'i' && *(temp+1) == 'd' && *(temp+2) == '=')
		{
			if(*(temp+3) == '"')
			{
				Idlen=Web_parse_line(temp+4,&beforeidlen,idvaluebuffer,&startatfteid,&IdType);
				if(Idlen!=0)
				{

					if(Idlen>startatfteid)
					{
						 Lenchange=Idlen-startatfteid;
					}
					beforeidlen+=(len+1);
				}		
			}
		}
		
		len++;
	}
	len++;

	if(IdType==0x00)
	{
		if (len+Lenchange<=  max_length)
		{	
			if(Idlen==0)
			{
				memcpy(buffer, data, len);
				* tembuflen+=len;

			}
			else
			{
				memcpy(buffer,data,beforeidlen);
				* tembuflen+=beforeidlen;
				strcpy(buffer+beforeidlen,idvaluebuffer);
				* tembuflen+=Idlen;
				memcpy(buffer+beforeidlen+Idlen,((char *)data+beforeidlen+startatfteid),(len-beforeidlen-startatfteid));
				* tembuflen+=(len-beforeidlen-startatfteid);			
			}
		}
		else
		{
			len = -1;
		}
	  }	
	 else if(IdType==0x01)
	 {
	 	if(beforeidlen+Lenchange<= max_length)	
	 	{
			if(Idlen==0)
			{
				memcpy(buffer, data, len);
				* tembuflen+=len;
			}
			else
			{
				memcpy(buffer,data,beforeidlen);				
				* tembuflen+=beforeidlen;
				strcpy(buffer+beforeidlen,idvaluebuffer);
				* tembuflen+=Idlen;
			}	 		
	 	}
		else
		{
	 		len = -1;	
		}
	 }
	 else
	 {
	 	len = -1;
	 }
     if(idvaluebuffer)
	 	mem_free(idvaluebuffer);
     return len; 
}

