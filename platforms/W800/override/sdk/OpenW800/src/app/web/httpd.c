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

/* This file is modified from the original version (httpd.c) as shipped with
 * lwIP version 1.3.0. Changes have been made to allow support for a
 * rudimentary server-side-include facility which will replace tags of the form
 * <!--#tag--> in any file whose extension is .shtml with strings provided by
 * an include handler whose pointer is provided to the module via function
 * http_set_ssi_handler(). Additionally, a simple common gateway interface
 * (CGI) handling mechanism has been added to allow clients to hook functions
 * to particular request URIs.
 *
 * To enable SSI support, define label INCLUDE_HTTPD_SSI in lwipopts.h.
 * To enable CGI support, define label INCLUDE_HTTPD_CGI in lwipopts.h.
 *
 */

/*
 * Notes about valid SSI tags
 * --------------------------
 *
 * The following assumptions are made about tags used in SSI markers:
 *
 * 1. No tag may contain '-' or whitespace characters within the tag name.
 * 2. Whitespace is allowed between the tag leadin "<!--#" and the start of
 *    the tag name and between the tag name and the leadout string "-->".
 * 3. The maximum tag name length is MAX_TAG_NAME_LEN, currently 8 characters.
 *
 * Notes on CGI usage
 * ------------------
 *
 * The simple CGI support offered here works with GET method requests only
 * and can handle up to 16 parameters encoded into the URI. The handler
 * function may not write directly to the HTTP output but must return a
 * filename that the HTTP server will send to the browser as a response to
 * the incoming CGI request.
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

//#include "lwip/debug.h"

//#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "lwip/memp.h"
#include "httpd.h"
#include "wm_wifi.h"
#include "wm_fwup.h"
#include "wm_watchdog.h"
#include "wm_params.h"
#include "wm_wifi_oneshot.h"
#include "wm_ram_config.h"

//#include "lwip/tcp.h"

#include "fs.h"

#include <string.h>
#include "httpd.h"
//#include "../../../utils/ustdlib.h"

#define WEB_SERVER_AUTHEN			0
const u8 SysSuperPass[] = "^$#^%&";  /* Shift <643657> */
extern u8 gwebcfgmode;
#if 0
#ifndef TRUE
#define TRUE ((u8)1)
#endif

#ifndef FALSE
#define FALSE ((u8)0)
#endif
#endif

typedef struct
{
    const char *name;
    u8 shtml;
} default_filename;

const default_filename g_psDefaultFilenames[] = {
  {"/index.html", FALSE },
  {"/index.html", FALSE },
};

#define NUM_DEFAULT_FILENAMES (sizeof(g_psDefaultFilenames) /                 \
                               sizeof(default_filename))

#ifdef INCLUDE_HTTPD_SSI
const char *g_pcSSIExtensions[] = {
  ".shtml", ".shtm", ".ssi"
};

#define NUM_SHTML_EXTENSIONS (sizeof(g_pcSSIExtensions) / sizeof(const char *))

enum tag_check_state {
    TAG_NONE,       /* Not processing an SSI tag */
    TAG_LEADIN,     /* Tag lead in "<!--#" being processed */
    TAG_FOUND,      /* Tag name being read, looking for lead-out start */
    TAG_LEADOUT,    /* Tag lead out "-->" being processed */
    TAG_SENDING     /* Sending tag replacement string */
};
#endif /* INCLUDE_HTTPD_SSI */
/////added by zcs//////
 struct http_recive {
		char * Httpd_recive_buf;
		char * PreBuf;
		int       charlen;
		u8  Valid;
 	};
////////adde end/////

struct http_state {
  int error;
  struct fs_file *handle;
  int file_flag;
#define FILEFLAG_FILTER 0x01  
  char *file;       /* Pointer to first unsent byte in buf. */
  char *buf;        /* File read buffer. */
#ifdef INCLUDE_HTTPD_SSI
  char *parsed;     /* Pointer to the first unparsed byte in buf. */
  char *tag_end;    /* Pointer to char after the closing '>' of the tag. */
  u32 parse_left; /* Number of unparsed bytes in buf. */
#endif
  u32 left;       /* Number of unsent bytes in buf. */
  int buf_len;      /* Size of file read buffer, buf. */
  u8 retries;
#ifdef INCLUDE_HTTPD_SSI
  u8 tag_check;   /* TRUE if we are processing a .shtml file else FALSE */
  u8 tag_index;   /* Counter used by tag parsing state machine */
  u8 tag_insert_len; /* Length of insert in string tag_insert */
  u8 tag_name_len; /* Length of the tag name in string tag_name */
  char tag_name[MAX_TAG_NAME_LEN + 1]; /* Last tag name extracted */
  char tag_insert[MAX_TAG_INSERT_LEN + 1]; /* Insert string for tag_name */
  enum tag_check_state tag_state; /* State of the tag processor */
#endif
#ifdef INCLUDE_HTTPD_CGI
  char *params[MAX_CGI_PARAMETERS]; /* Params extracted from the request URI */
  char *param_vals[MAX_CGI_PARAMETERS]; /* Values for each extracted param */
#endif

  int recv_state;
  int recv_content_len;
  char recv_boundry[128];
 struct http_recive http_recive_request;
  struct pbuf *RecvPbuf;
  int RecvOffset;
//  struct tcp_pcb *pcb;
};

#ifdef INCLUDE_HTTPD_SSI
/* SSI insert handler function pointer. */
tSSIHandler g_pfnSSIHandler = NULL;
int g_iNumTags = 0;
const char **g_ppcTags = NULL;

#define LEN_TAG_LEAD_IN 5
const char * const g_pcTagLeadIn = "<!--#";

#define LEN_TAG_LEAD_OUT 3
const char * const g_pcTagLeadOut = "-->";
#endif /* INCLUDE_HTTPD_SSI */

#ifdef INCLUDE_HTTPD_CGI
/* CGI handler information */
const tCGI *g_pCGIs = NULL;
int g_iNumCGIs = 0;
#endif /* INCLUDE_HTTPD_CGI */
//static u32 session_id;

 const  char GpucHttpHead_Authen[] = {\
    "HTTP/1.1 401 Unauthorized\r\n"
    "Data:Thu,03 Jan 2010 10:00:00 GMT\r\n"
    "Server: actHttp/1.0 ActionTop Corporation\r\n"
    "Accept-Ranges: bytes\r\n"
    "WWW-Authenticate: Basic realm=\".\"\r\n"
    "Content-Type:text/html\r\n"
    "Connection:close\r\n"
    "\r\n"
};

/*-----------------------------------------------------------------------------------*/
static void
conn_err(void *arg, err_t err)
{
  struct http_state *hs;

  LWIP_UNUSED_ARG(err);

  if(arg)
  {
      hs = arg;
#if 0
	  if (hs->pcb){
	  	  DEBUG_PRINT("connerr:%d, 0x%x\n", err, hs->pcb);
		  tcp_arg(hs->pcb, NULL);
		  tcp_sent(hs->pcb, NULL);
		  tcp_recv(hs->pcb, NULL);
		  tcp_err(hs->pcb, NULL);
		  tcp_poll(hs->pcb,NULL,4);
		  tcp_close(hs->pcb);
	  }
#endif	  
      if(hs->handle) {
        fs_close(hs->handle);
        hs->handle = NULL;
      }
      if(hs->buf)
      {
          mem_free(hs->buf);
      }
      mem_free(hs);
  }
}
/*-----------------------------------------------------------------------------------*/
static void
close_conn(struct tcp_pcb *pcb, struct http_state *hs)
{
  DEBUG_PRINT("Closing connection 0x%08x\n\r", (u32)pcb);
  if (pcb){
	  tcp_arg(pcb, NULL);
	  tcp_sent(pcb, NULL);
	  tcp_recv(pcb, NULL);
	  tcp_err(pcb, NULL);
	  tcp_poll(pcb,NULL,4);
  }
  if(hs->handle) {
    fs_close(hs->handle);
    hs->handle = NULL;
  }
  if(hs->buf)
  {
    mem_free(hs->buf);
  }
  mem_free(hs);
  if (pcb)
  	tcp_close(pcb);
#if TLS_CONFIG_WEB_SERVER_MODE
    if (gwebcfgmode == 1)
    {
        tls_oneshot_send_web_connect_msg();
        gwebcfgmode = 2;
    }
#endif  
}
/*-----------------------------------------------------------------------------------*/
#ifdef INCLUDE_HTTPD_CGI
static int
extract_uri_parameters(struct http_state *hs, char *params)
{
  char *pair;
  char *equals;
  int loop;

  /* If we have no parameters at all, return immediately. */
  if(!params || (params[0] == '\0')) {
      return(0);
  }

  /* Get a pointer to our first parameter */
  pair = params;

  /*
   * Parse up to MAX_CGI_PARAMETERS from the passed string and ignore the
   * remainder (if any)
   */
  for(loop = 0; (loop < MAX_CGI_PARAMETERS) && pair; loop++) {

    /* Save the name of the parameter */
    hs->params[loop] = pair;

    /* Remember the start of this name=value pair */
    equals = pair;

    /* Find the start of the next name=value pair and replace the delimiter
     * with a 0 to terminate the previous pair string.
     */
    pair = strchr(pair, '&');
    if(pair) {
      *pair = '\0';
      pair++;
    }

    /* Now find the '=' in the previous pair, replace it with '\0' and save
     * the parameter value string.
     */
    equals = strchr(equals, '=');
    if(equals) {
      *equals = '\0';
      hs->param_vals[loop] = equals + 1;
    } else {
      hs->param_vals[loop] = NULL;
    }
  }

  return(loop);
}
#endif /* INCLUDE_HTTPD_CGI */

/*-----------------------------------------------------------------------------------*/
#ifdef INCLUDE_HTTPD_SSI
static void
get_tag_insert(struct http_state *hs)
{
  int loop;

  if(g_pfnSSIHandler && g_ppcTags && g_iNumTags) {

    /* Find this tag in the list we have been provided. */
    for(loop = 0; loop < g_iNumTags; loop++)
    {
      if(strcmp(hs->tag_name, g_ppcTags[loop]) == 0) {
        hs->tag_insert_len = g_pfnSSIHandler(loop, hs->tag_insert,
                                             MAX_TAG_INSERT_LEN);
        return;
      }
    }
  }

  /* If we drop out, we were asked to serve a page which contains tags that
   * we don't have a handler for. Merely echo back the tags with an error
   * marker.
   */
  usnprintf(hs->tag_insert, MAX_TAG_INSERT_LEN + 1,
            "<b>***UNKNOWN TAG %s***</b>", hs->tag_name);
  hs->tag_insert_len = strlen(hs->tag_insert);
}
#endif /* INCLUDE_HTTPD_SSI */

extern int fs_read_line(const void *data, char  *buffer, int  max_length,int * tembuflen);
void send_data_to_sys(struct http_state *hs);
void send_jump_html(struct http_state *hs,struct tcp_pcb *pcb);
void send_error_html(struct http_state *hs,struct tcp_pcb *pcb, int error);

/*-----------------------------------------------------------------------------------*/
static err_t
http_sent(void *arg, struct tcp_pcb *pcb, u16 len);

static void
send_data(struct tcp_pcb *pcb, struct http_state *hs)
{
  err_t err;
  u16 len;
  u16 filelen=0;
 
  u8 data_to_send = FALSE;
  char * Temp=NULL;
  /* Have we run out of file data to send? If so, we need to read the next
   * block from the file.
   */
  if(hs->left == 0)
  {
    int count;

    /* Do we already have a send buffer allocated? */
    if(hs->buf) {
      /* Yes - get the length of the buffer */
      count = hs->buf_len;
    }
    else {
      /* We don't have a send buffer so allocate one up to 2*mss bytes long. */
      count = 2*pcb->mss;
      do {
        hs->buf = mem_malloc(count);
        if(hs->buf) {
          hs->buf_len = count;
          break;
        }
        count = count / 2;
      }while (count > 100);

      /* Did we get a send buffer? If not, return immediately. */
      if(hs->buf == NULL) {
        DEBUG_PRINT("No buff\n\r");
        return;
      }
    }

    /* Do we have a valid file handle? */
    if(hs->handle == NULL)
    {
        //
        // No - close the connection.
        //
        close_conn(pcb, hs);
        return;
    }

    /* Read a block of data from the file. */
//    DEBUG_PRINT("Trying to read %d bytes.\n", count);

   count = fs_read(hs->handle, hs->buf, count);

    /// count = fs_read_line(hs->handle, hs->buf, count);
	
    if(count < 0) {
      /* We reached the end of the file so this request is done */
      DEBUG_PRINT("End of file here.\n\r");
      close_conn(pcb, hs);
      return;
    }

    /* Set up to send the block of data we just read */
    DEBUG_PRINT("Read %d bytes.\n\r", count);
    hs->left = count;
    hs->file = hs->buf;
#ifdef INCLUDE_HTTPD_SSI
    hs->parse_left = count;
    hs->parsed = hs->buf;
#endif
  }

#ifdef INCLUDE_HTTPD_SSI
  if(!hs->tag_check) {
#endif
      /* We are not processing an SHTML file so no tag checking is necessary.
       * Just send the data as we received it from the file.
       */

      /* We cannot send more data than space available in the send
         buffer. */
#if 0
      if (tcp_sndbuf(pcb) < hs->left) {
        len = tcp_sndbuf(pcb);
      } else {
        len = hs->left;
        LWIP_ASSERT("hs->left did not fit into u16!", (len == hs->left));
      }
      if(len > (2*pcb->mss)) {
        len = 2*pcb->mss;
      }
#else
      if (tcp_sndbuf(pcb) < hs->left) {
        len = tcp_sndbuf(pcb);
      } else {
        len = hs->left;
        LWIP_ASSERT("hs->left did not fit into u16!", (len == hs->left));
      }
      if(len > (4*pcb->mss)) {
        len = 4*pcb->mss;
      }
#endif
if (hs->file_flag & FILEFLAG_FILTER)
{
	int memlen=len+256;
	Temp=mem_malloc(memlen);
	if(Temp==NULL)
	{
	   err = ERR_MEM;
	   return;
	}
	
      do {
#if 1	  	
       	int templen=0,j=0;
		filelen=0;
		memset(Temp,0,memlen);		
		do
		{
                   j= fs_read_line(hs->file+filelen, Temp+templen,len-templen,&templen);/////          
		     if(j>=0)	
		     {
			   filelen+=j;
		      }
		}while(j>=0);
		//DBGPRINT("###kevin debug tcp_write = %s\n\r",Temp);
	       err = tcp_write(pcb,Temp, strlen(Temp), TCP_WRITE_FLAG_COPY);
#else	
       err = tcp_write(pcb, hs->file, len, 0);
#endif
	if (err == ERR_MEM) {
          len /= 2;
        }
      } while (err == ERR_MEM && filelen > 1);
if(Temp)
	mem_free(Temp);
 }
	  else
	  {
		  filelen = len;
		  err = tcp_write(pcb, hs->file, filelen, 0);
	  }

      if (err == ERR_OK) {
        data_to_send = TRUE;
        hs->file += filelen;
        hs->left -= filelen;
      }
#ifdef INCLUDE_HTTPD_SSI
  } else {
    /* We are processing an SHTML file so need to scan for tags and replace
     * them with insert strings. We need to be careful here since a tag may
     * straddle the boundary of two blocks read from the file and we may also
     * have to split the insert string between two tcp_write operations.
     */

    /* How much data could we send? */
    len = tcp_sndbuf(pcb);

    /* Do we have remaining data to send before parsing more? */
    if(hs->parsed > hs->file) {
        /* We cannot send more data than space available in the send
           buffer. */
        if (tcp_sndbuf(pcb) < (hs->parsed - hs->file)) {
          len = tcp_sndbuf(pcb);
        } else {
          len = (hs->parsed - hs->file);
          LWIP_ASSERT("Data size did not fit into u16!",
                      (hs->parsed - hs->file));
        }
        if(len > (2*pcb->mss)) {
          len = 2*pcb->mss;
        }

        do {
          DEBUG_PRINT("Sending %d bytes\n\r", len);
          err = tcp_write(pcb, hs->file, len, 0);
          if (err == ERR_MEM) {
            len /= 2;
          }
        } while (err == ERR_MEM && len > 1);

        if (err == ERR_OK) {
          data_to_send = TRUE;
          hs->file += len;
          hs->left -= len;
        }

        //
        // If the send buffer is full, return now.
        //
        if(tcp_sndbuf(pcb) == 0) {
          if(data_to_send) {
		  tcp_sent(pcb, http_sent);
            tcp_output(pcb);
            DEBUG_PRINT("Output\n\r");
          }
          return;
        }
    }

    DEBUG_PRINT("State %d, %d left\n\r", hs->tag_state, hs->parse_left);

    /* We have sent all the data that was already parsed so continue parsing
     * the buffer contents looking for SSI tags.
     */
    while(hs->parse_left) {
      switch(hs->tag_state) {
        case TAG_NONE:
          /* We are not currently processing an SSI tag so scan for the
           * start of the lead-in marker.
           */
          if(*hs->parsed == g_pcTagLeadIn[0])
          {
              /* We found what could be the lead-in for a new tag so change
               * state appropriately.
               */
              hs->tag_state = TAG_LEADIN;
              hs->tag_index = 1;
          }

          /* Move on to the next character in the buffer */
          hs->parse_left--;
          hs->parsed++;
          break;

        case TAG_LEADIN:
          /* We are processing the lead-in marker, looking for the start of
           * the tag name.
           */

          /* Have we reached the end of the leadin? */
          if(hs->tag_index == LEN_TAG_LEAD_IN) {
              hs->tag_index = 0;
              hs->tag_state = TAG_FOUND;
          } else {
            /* Have we found the next character we expect for the tag leadin?
             */
            if(*hs->parsed == g_pcTagLeadIn[hs->tag_index]) {
              /* Yes - move to the next one unless we have found the complete
               * leadin, in which case we start looking for the tag itself
               */
              hs->tag_index++;
            } else {
              /* We found an unexpected character so this is not a tag. Move
               * back to idle state.
               */
              hs->tag_state = TAG_NONE;
            }

            /* Move on to the next character in the buffer */
            hs->parse_left--;
            hs->parsed++;
          }
          break;

        case TAG_FOUND:
          /* We are reading the tag name, looking for the start of the
           * lead-out marker and removing any whitespace found.
           */

          /* Remove leading whitespace between the tag leading and the first
           * tag name character.
           */
          if((hs->tag_index == 0) && ((*hs->parsed == ' ') ||
             (*hs->parsed == '\t') || (*hs->parsed == '\n') ||
             (*hs->parsed == '\r')))
          {
              /* Move on to the next character in the buffer */
              hs->parse_left--;
              hs->parsed++;
              break;
          }

          /* Have we found the end of the tag name? This is signalled by
           * us finding the first leadout character or whitespace */
          if((*hs->parsed == g_pcTagLeadOut[0]) ||
             (*hs->parsed == ' ') || (*hs->parsed == '\t') ||
             (*hs->parsed == '\n')  || (*hs->parsed == '\r')) {

            if(hs->tag_index == 0) {
              /* We read a zero length tag so ignore it. */
              hs->tag_state = TAG_NONE;
            } else {
              /* We read a non-empty tag so go ahead and look for the
               * leadout string.
               */
              hs->tag_state = TAG_LEADOUT;
              hs->tag_name_len = hs->tag_index;
              hs->tag_name[hs->tag_index] = '\0';
              if(*hs->parsed == g_pcTagLeadOut[0]) {
                hs->tag_index = 1;
              } else {
                hs->tag_index = 0;
              }
            }
          } else {
            /* This character is part of the tag name so save it */
            if(hs->tag_index < MAX_TAG_NAME_LEN) {
              hs->tag_name[hs->tag_index++] = *hs->parsed;
            } else {
              /* The tag was too long so ignore it. */
              hs->tag_state = TAG_NONE;
            }
          }

          /* Move on to the next character in the buffer */
          hs->parse_left--;
          hs->parsed++;

          break;

        /*
         * We are looking for the end of the lead-out marker.
         */
        case TAG_LEADOUT:
          /* Remove leading whitespace between the tag leading and the first
           * tag leadout character.
           */
          if((hs->tag_index == 0) && ((*hs->parsed == ' ') ||
             (*hs->parsed == '\t') || (*hs->parsed == '\n') ||
             (*hs->parsed == '\r')))
          {
            /* Move on to the next character in the buffer */
            hs->parse_left--;
            hs->parsed++;
            break;
          }

          /* Have we found the next character we expect for the tag leadout?
           */
          if(*hs->parsed == g_pcTagLeadOut[hs->tag_index]) {
            /* Yes - move to the next one unless we have found the complete
             * leadout, in which case we need to call the client to process
             * the tag.
             */

            /* Move on to the next character in the buffer */
            hs->parse_left--;
            hs->parsed++;

            if(hs->tag_index == (LEN_TAG_LEAD_OUT - 1)) {
              /* Call the client to ask for the insert string for the
               * tag we just found.
               */
              get_tag_insert(hs);

              /* Next time through, we are going to be sending data
               * immediately, either the end of the block we start
               * sending here or the insert string.
               */
              hs->tag_index = 0;
              hs->tag_state = TAG_SENDING;
              hs->tag_end = hs->parsed;

              /* If there is any unsent data in the buffer prior to the
               * tag, we need to send it now.
               */
              if(hs->tag_end > hs->file)
              {
                /* How much of the data can we send? */
                if(len > hs->tag_end - hs->file) {
                    len = hs->tag_end - hs->file;
                }

                do {
                  DEBUG_PRINT("Sending %d bytes\n\r", len);
                  err = tcp_write(pcb, hs->file, len, 0);
                  if (err == ERR_MEM) {
                    len /= 2;
                  }
                } while (err == ERR_MEM && (len > 1));

                if (err == ERR_OK) {
                  data_to_send = TRUE;
                  hs->file += len;
                  hs->left -= len;
                }
              }
            } else {
              hs->tag_index++;
            }
          } else {
              /* We found an unexpected character so this is not a tag. Move
               * back to idle state.
               */
              hs->parse_left--;
              hs->parsed++;
              hs->tag_state = TAG_NONE;
          }
          break;

        /*
         * We have found a valid tag and are in the process of sending
         * data as a result of that discovery. We send either remaining data
         * from the file prior to the insert point or the insert string itself.
         */
        case TAG_SENDING:
          /* Do we have any remaining file data to send from the buffer prior
           * to the tag?
           */
          if(hs->tag_end > hs->file)
          {
            /* How much of the data can we send? */
            if(len > hs->tag_end - hs->file) {
              len = hs->tag_end - hs->file;
            }

            do {
              DEBUG_PRINT("Sending %d bytes\n\r", len);
              err = tcp_write(pcb, hs->file, len, 0);
              if (err == ERR_MEM) {
                len /= 2;
              }
            } while (err == ERR_MEM && (len > 1));

            if (err == ERR_OK) {
              data_to_send = TRUE;
              hs->file += len;
              hs->left -= len;
            }
          } else {
            /* Do we still have insert data left to send? */
            if(hs->tag_index < hs->tag_insert_len) {
              /* We are sending the insert string itself. How much of the
               * insert can we send? */
              if(len > (hs->tag_insert_len - hs->tag_index)) {
                len = (hs->tag_insert_len - hs->tag_index);
              }

              do {
                DEBUG_PRINT("Sending %d bytes\n\r", len);
                /*
                 * Note that we set the copy flag here since we only have a
                 * single tag insert buffer per connection. If we don't do
                 * this, insert corruption can occur if more than one insert
                 * is processed before we call tcp_output.
                 */
                err = tcp_write(pcb, &(hs->tag_insert[hs->tag_index]), len, 1);
                if (err == ERR_MEM) {
                  len /= 2;
                }
              } while (err == ERR_MEM && (len > 1));

              if (err == ERR_OK) {
                data_to_send = TRUE;
                hs->tag_index += len;
                return;
              }
            } else {
              /* We have sent all the insert data so go back to looking for
               * a new tag.
               */
              DEBUG_PRINT("Everything sent.\n\r");
              hs->tag_index = 0;
              hs->tag_state = TAG_NONE;
          }
        }
      }
    }

    /*
     * If we drop out of the end of the for loop, this implies we must have
     * file data to send so send it now. In TAG_SENDING state, we've already
     * handled this so skip the send if that's the case.
     */
    if((hs->tag_state != TAG_SENDING) && (hs->parsed > hs->file)) {
      /* We cannot send more data than space available in the send
         buffer. */
      if (tcp_sndbuf(pcb) < (hs->parsed - hs->file)) {
        len = tcp_sndbuf(pcb);
      } else {
        len = (hs->parsed - hs->file);
        LWIP_ASSERT("Data size did not fit into u16!",
                    (hs->parsed - hs->file));
      }
      if(len > (2*pcb->mss)) {
        len = 2*pcb->mss;
      }

      do {
        DEBUG_PRINT("Sending %d bytes\n\r", len);
        err = tcp_write(pcb, hs->file, len, 0);
        if (err == ERR_MEM) {
          len /= 2;
        }
      } while (err == ERR_MEM && len > 1);

      if (err == ERR_OK) {
        data_to_send = TRUE;
        hs->file += len;
        hs->left -= len;
      }
    }
  }
#endif /* INCLUDE_HTTPD_SSI */

  /* If we wrote anything to be sent, go ahead and send it now. */
  if(data_to_send) {
//    DEBUG_PRINT("tcp_output\n");
    tcp_sent(pcb, http_sent);
    tcp_output(pcb);
  }

  ///DEBUG_PRINT("send_data end.\n");
}

/*-----------------------------------------------------------------------------------*/
static err_t
http_poll(void *arg, struct tcp_pcb *pcb)
{
  struct http_state *hs;

  hs = arg;

 DEBUG_PRINT("http_poll 0x%08x\n\r", (u32)pcb);

    if (hs == NULL) {
    /*    printf("Null, close\n");*/
    tcp_abort(pcb);
    return ERR_ABRT;
} else {
    if (hs->recv_state == 0){
	    ++hs->retries;
	    if (hs->retries >= 10) {
	      tcp_abort(pcb);
		  DEBUG_PRINT("retry out, abort\n\r");
	      return ERR_ABRT;
	    }
	    send_data(pcb, hs);
    }	
    else{
	if (hs->error == 0){
	        send_data_to_sys(hs);	
	}
	else if (hs->error == 1){
          send_jump_html(hs,pcb);
       }
	else{
          send_error_html(hs, pcb, hs->error);
	}

	        #if 0  /* d Deactivated  2010-08-18,  */
	        if ((hs->recv_state != 0)&&(hs->recv_content_len <= 0)&&(hs->RecvPbuf == NULL)){
		     hs->recv_state = 0;		
	            Sending_jump_html(hs,pcb);
		}		
	        #endif
    } 	
  }

  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
static err_t
http_sent(void *arg, struct tcp_pcb *pcb, u16 len)
{
  struct http_state *hs;

//  DEBUG_PRINT("http_sent 0x%08x\n", pcb);


  LWIP_UNUSED_ARG(len);

  hs = arg;

  hs->retries = 0;

  /* Temporarily disable send notifications */
  tcp_sent(pcb, NULL);

  
  send_data(pcb, hs);

  /* Reenable notifications. */
//  if(TcpConnected==1)
//  tcp_sent(pcb, http_sent);

  return ERR_OK;
}
#if WEB_SERVER_AUTHEN
/****************************************************************************
 *
 > $Function: decodeBase64()
 *
 > $Description: Decode a base 64 data stream as per rfc1521.
 *    Note that the rfc states that none base64 chars are to be ignored.
 *    Since the decode always results in a shorter size than the input, it is
 *    OK to pass the input arg as an output arg.
 *
 * $Parameter:
 *      (char *) Data . . . . A pointer to a base64 encoded string.
 *                            Where to place the decoded data.
 *
 * $Return: void
 *
 * $Errors: None
 *
 ****************************************************************************/
static void decodeBase64(char *Data)
{

	const unsigned char *in = (const unsigned char *)Data;
	// The decoded size will be at most 3/4 the size of the encoded
	unsigned long ch = 0;
	int i = 0;

	while (*in) {
		int t = *in++;

		if (t >= '0' && t <= '9')
			t = t - '0' + 52;
		else if (t >= 'A' && t <= 'Z')
			t = t - 'A';
		else if (t >= 'a' && t <= 'z')
			t = t - 'a' + 26;
		else if (t == '+')
			t = 62;
		else if (t == '/')
			t = 63;
		else if (t == '=')
			t = 0;
		else
			continue;

		ch = (ch << 6) | t;
		i++;
		if (i == 4) {
			*Data++ = (char) (ch >> 16);
			*Data++ = (char) (ch >> 8);
			*Data++ = (char) ch;
			i = 0;
		}
	}
	*Data = 0;
}
#endif
void send_jump_html(struct http_state *hs,struct tcp_pcb *pcb)
{
   struct fs_file *file;
   
    file = fs_open("/jump.html");		 
    if(file == NULL) 
    {
            file = fs_open("/404.html");
    }
    hs->handle = file;
    hs->file = file->data;


    LWIP_ASSERT("File length must be positive!", (file->len >= 0));
    hs->left = file->len;
    hs->retries = 0;

     DEBUG_PRINT("File length = %d\n\r", file->len); 

   ///     pbuf_free(p);

    send_data(pcb, hs);
	
}

static tls_os_timer_t *restart_tmr = NULL;
static void restart_tmr_handler(void *ptmr, void *parg)
{
	tls_sys_set_reboot_reason(REBOOT_REASON_ACTIVE);
	tls_sys_reset();
}
static void resethandler()
{
    tls_os_status_t err;
    if(restart_tmr == NULL)
    {
        err = tls_os_timer_create(&restart_tmr,
                restart_tmr_handler,
                (void *)0,
                HZ/10,  /* 100 ms */
                FALSE,
                NULL);
    
        if (!err)
            tls_os_timer_start(restart_tmr); 
    }
    else
    {
        tls_os_timer_change(restart_tmr, HZ/10);
    }
}

 u8   extract_html_recive(char * html_data,  struct http_state *hs,struct tcp_pcb *pcb)
 {
   int i;
   int loop;
   struct fs_file *file;
  // int UrlBufSize = HTTP_MAX_REQUEST_LEN;
   ///char Url[HTTP_MAX_REQUEST_LEN];
   char * Url=NULL;
  #ifdef INCLUDE_HTTPD_CGI
  int count;
  char *params;
  #endif
#if WEB_SERVER_AUTHEN
  char * AuthChar=NULL;
#endif
  u8 NeedRestart=0;
    u8 mode;

	
	if (strncmp(html_data, "GET ", 4) == 0) 
	{
		DEBUG_PRINT("extract_html_recive GET start");
#if WEB_SERVER_AUTHEN
	    AuthChar= strstr(html_data,"Authorization:");

	    if(AuthChar==NULL)
	    {
	    	 tcp_write(pcb,(char *)GpucHttpHead_Authen, strlen(GpucHttpHead_Authen), TCP_WRITE_FLAG_COPY);
		 tcp_sent(pcb, http_sent);
	        tcp_output(pcb);	 
		 close_conn(pcb, hs);	   
		return 0;
	    }
	    else
	    {
	       u8 password[6];
              tls_param_get(TLS_PARAM_ID_PASSWORD, password, FALSE);
	    	decodeBase64(AuthChar+sizeof("Authorization: Basic"));
		if(strncmp((char *)AuthChar+sizeof("Authorization: Basic"),"admin:",6)==0&&strncmp((char *)AuthChar+sizeof("Authorization: Basic admin"),(char *)SysSuperPass,6)==0)
		{
			DEBUG_PRINT("PassWord is SysSuperPass\n\r" );	
		}	
		else if(strncmp((char *)AuthChar+sizeof("Authorization: Basic"),"admin:",6)!=0||strncmp((char *)AuthChar+sizeof("Authorization: Basic admin"),(char *)password,6)!=0)
		{
			DEBUG_PRINT("PassWord is Error \n\r" );
	    	 	tcp_write(pcb,(char *)GpucHttpHead_Authen, strlen(GpucHttpHead_Authen), TCP_WRITE_FLAG_COPY);
		 	tcp_sent(pcb, http_sent);
	        	tcp_output(pcb);		
		       close_conn(pcb, hs);	   
			return 0;
		}	
	    }
#endif		
        /*
         * We have a GET request. Find the end of the URI by looking for the
         * HTTP marker. We can't just use strstr to find this since the request
         * came from an outside source and we can't be sure that it is
         * correctly formed. We need to make sure that our search is bounded
         * by the packet length so we do it manually. If we don't find " HTTP",
         * assume the request is invalid and close the connection.
         */	
//        DEBUG_PRINT("Request:\n%s\n", Url); 
        Url=&html_data[4];
        for(i = 0; i <= (strlen(html_data)-4 - 5); i++) 
	 {
            if ((Url[i] == ' ') && (Url[i + 1] == 'H') &&
                (Url[i + 2] == 'T') && (Url[i + 3] == 'T') &&
                (Url[i + 4] == 'P')) 
            {
              Url[i] = 0;   
              break;
            }                    
        }
        DEBUG_PRINT("Request Url: %s\n", Url); 
	 if(i == (strlen(html_data)-4 - 4))
        {
          /* We failed to find " HTTP" in the request so assume it is invalid */
          DEBUG_PRINT("Invalid GET request. Closing.\n\r");
          close_conn(pcb, hs);
        }          
        
#ifdef INCLUDE_HTTPD_SSI
        /*
         * By default, assume we will not be processing server-side-includes
         * tags
         */
        hs->tag_check = FALSE;
#endif

        /*
         * Have we been asked for the default root file?
         */
        if((Url[0] == '/') &&  (Url[1] == 0)) {
          /*
           * Try each of the configured default filenames until we find one
           * that exists.
           */
#if 0
          for(loop = 0; loop < NUM_DEFAULT_FILENAMES; loop++) {
//            DEBUG_PRINT("Looking for %s...\n", g_psDefaultFilenames[loop].name);
            file = fs_open((char *)g_psDefaultFilenames[loop].name);
            if(file != NULL) {
              DEBUG_PRINT("Opened %s.\n\r", (char *)g_psDefaultFilenames[loop].name);
#ifdef INCLUDE_HTTPD_SSI
              hs->tag_check = g_psDefaultFilenames[loop].shtml;
#endif
              break;
            }
          }
#else
          tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void* )&mode, FALSE);
          loop= (mode == IEEE80211_MODE_AP) ? 1 : 0;
          file = fs_open((char *)g_psDefaultFilenames[loop].name);
		DEBUG_PRINT("open file : %s\r\n", (char *)g_psDefaultFilenames[loop].name);
#endif

          if(file == NULL) {
            /* None of the default filenames exist so send back a 404 page */
            file = fs_open("/404.html");
#ifdef INCLUDE_HTTPD_SSI
            hs->tag_check = FALSE;
#endif
          }
        } else {
          /* No - we've been asked for a specific file. */
#ifdef INCLUDE_HTTPD_CGI
          /* First, isolate the base URI (without any parameters) */
          params = strchr(Url, '?');
          if(params) {
            *params = '\0';
            params++;
          }

          /* Does the base URI we have isolated correspond to a CGI handler? */
          if(g_iNumCGIs && g_pCGIs) {
            for(i = 0; i < g_iNumCGIs; i++) {
              if(strcmp(Url, g_pCGIs[i].pcCGIName) == 0) {
                /*
                 * We found a CGI that handles this URI so extract the
                 * parameters and call the handler.
                 */
                 count = extract_uri_parameters(hs, params);
                 strcpy(Url, g_pCGIs[i].pfnCGIHandler(i, count, hs->params, hs->param_vals,&NeedRestart));
		   if(NeedRestart==1)
		   {
			send_jump_html(hs,pcb);
			DEBUG_PRINT("Sending jump html\n\r"); 
      		   	resethandler();
			return 0;
		   }
                 break;
              }
            }
          }
#endif
	  DEBUG_PRINT("Opening %s\n\r", Url);

         	 file = fs_open(Url);
		  //if (strstr(Url, "hed"))
		  {
			hs->file_flag |= FILEFLAG_FILTER;
		  }	 			 
          if(file == NULL) {
            if (tls_wifi_get_oneshot_flag()) {
                file = fs_open("/index.html");
                if (NULL == file)
                    file = fs_open("/404.html");
            } else {
                file = fs_open("/404.html");
            }
          }
#ifdef INCLUDE_HTTPD_SSI
          else {
            /*
             * See if we have been asked for an shtml file and, if so,
             * enable tag checking.
             */
            hs->tag_check = FALSE;
            for(loop = 0; loop < NUM_SHTML_EXTENSIONS; loop++) {
              if(ustrstr(Url, g_pcSSIExtensions[loop])) {              
                hs->tag_check = TRUE;
                break;
              }
            }
          }
#endif /* INCLUDE_HTTP_SSI */
        }

#ifdef INCLUDE_HTTPD_SSI
        hs->tag_index = 0;
        hs->tag_state = TAG_NONE;
        hs->parsed = file->data;
        hs->parse_left = file->len;
        hs->tag_end = file->data;
#endif
        hs->handle = file;
        hs->file = file->data;


        LWIP_ASSERT("File length must be positive!", (file->len >= 0));
        hs->left = file->len;
        hs->retries = 0;

        DEBUG_PRINT("File: data = 0x%x,  length = %d\n\r", file->data, file->len); 

   ///     pbuf_free(p);

        send_data(pcb, hs);

        /* Tell TCP that we wish be to informed of data that has been
           successfully sent by a call to the http_sent() function. */
           
//        tcp_sent(pcb, http_sent);
      }
	else{
		if (hs->recv_state == 0){
			if (strncmp(html_data, "POST ", 4) == 0) {
				char *ptr;
				ptr = strstr(html_data, "boundary=");
				if (ptr){
					sscanf(ptr, "boundary=%s\r\n", hs->recv_boundry);
		        		DEBUG_PRINT("boundary=%s\r\n", hs->recv_boundry); 
					ptr = strstr(html_data, "Content-Length");
					if (ptr){
						sscanf(ptr, "Content-Length: %d\r\n", &hs->recv_content_len);	
			        		DEBUG_PRINT("Content-Length=%d\r\n", hs->recv_content_len); 
						hs->recv_state = 1;
					}
				}	
			}	 
		}
		else{
//			if (strstr(html_data, "Content-Type: application/octet-stream")) {
			if (strstr(html_data, hs->recv_boundry)) {
#if 0
				TSYSC_MSG *msg;
				msg = OSMMalloc(sizeof(TSYSC_MSG));
				if (msg == NULL) {
					return 0;
				}
				memset(msg, 0, sizeof(TSYSC_MSG));
				msg->Id = SYSC_MSG_UPDATE_FW;
				msg->Len = 0;
				msg->Argc = (u32)hs;
				OSQPost(Que_Sys, (void *)msg);
#endif
	 			hs->recv_state = 2;
	 			//tls_fwup_reg_complete_callback(http_fwup_done_callback);
				//session_id = tls_fwup_enter(TLS_FWUP_IMAGE_SRC_WEB);
	        		//DEBUG_PRINT("begin to recv octet-stream %d\r\n", session_id); 
			}
		}
	}

	 #if 0  /* d Deactivated  2010-08-17,  */
	 else {
        data = html_data;
        memset(PlainBuf, 0, PLAIN_FLAG_LEN * 2);
        memset(FwEndBuf, 0, FWEND_FLAG_LEN * 2);
        DEBUG_PRINT("\nhere here Post:\n\r");
        for (j = pNext->tot_len; j >= pNext->len; \
               j -= pNext->len, pNext = pNext->next, data = pNext->payload)
        {       
 
        }		
        pbuf_free(p);
        close_conn(pcb, hs);	   
      }
	 #endif
	return 0;
 }


void send_data_to_sys(struct http_state *hs)
{
	int len;
	#if 0
	TSYSC_MSG *msg;
	OS_Q_DATA qinfo;
	#endif
	struct pbuf *p;
	struct pbuf *cur_p;
	int offset = hs->RecvOffset;
	

	if (hs->RecvPbuf == NULL)
	{
		return;
	}
	p = (struct pbuf *)hs->RecvPbuf;
	len = p->tot_len - hs->RecvOffset;
	DEBUG_PRINT("send_data_to_sys len=%d\n", len);
	if (len <= 0)
	{
		pbuf_free(p);
		hs->RecvPbuf = NULL;
		hs->RecvOffset = 0;
		return;
	}
	cur_p = p;
	while(cur_p)
	{
		if(cur_p->len <= offset)
		{
			offset -= cur_p->len;
		}
		else
		{
			//tls_fwup_request_sync(session_id, (u8 *)(cur_p->payload) + offset, (u32)(cur_p->len - offset));
			offset = 0;
		}
		cur_p = cur_p->next;
	}
	pbuf_free(p);
	hs->RecvPbuf = NULL;
	hs->RecvOffset = 0;
	#if 0
	memset(&qinfo, 0, sizeof(OS_Q_DATA));
	OSQQuery (Que_Sys, &qinfo);
	if (qinfo.OSNMsgs >= qinfo.OSQSize)
	{
//	if (qinfo.OSNMsgs >= 8){
		/* Queue if full */
		return;
	}

	len *= 2;	/* It will be divided by 2 as follow */

	do
	{
		len >>= 1;
		msg = OSMMalloc(sizeof(TSYSC_MSG) + len);
	}while((!msg)&&(len > 128));

	if (msg)
	{
		msg->Id = SYSC_MSG_UPDATE_FW;
		msg->Argc = (u32)hs;
		msg->Len = len;
		pbuf_copy_partial(p, msg->Argv, len, hs->RecvOffset);
		OSQPost(Que_Sys, (void *)msg);
		hs->RecvOffset += len;
		if (hs->RecvOffset >= p->tot_len)
		{
			pbuf_free(p);
			hs->RecvPbuf = NULL;
			hs->RecvOffset = 0;
		}
	}
	#endif
}

void send_error_html(struct http_state *hs,struct tcp_pcb *pcb, int error)
{
	char *errstr[] = {
		NULL,
		"invalid firmware file",
		"program flash failed",
		"varify incorrect",
	};
	char *html = mem_malloc(128);

	if (html){
		sprintf(html, "HTTP/1.1 200 OK\r\n"
			"<head>\r\n"
			"<title>error</title>\r\n"
			"</head>\r\n"
			"<body>\r\n"
			"error:%s"
			"</body>\r\n"
			"</html>\r\n"
			"\r\n\r\n", errstr[-error]);

		tcp_write(pcb,html, strlen(html), TCP_WRITE_FLAG_COPY);
		tcp_sent(pcb, http_sent);
		tcp_output(pcb);	 
		mem_free(html);
	}
	 close_conn(pcb, hs);	   
}

/*-----------------------------------------------------------------------------------*/
static err_t
http_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
    struct http_state *hs;
    char * temprecive=NULL;
    char * HtmlBegin=NULL, *HtmlEnd=NULL;
    int HtmlLen;		

//  DEBUG_PRINT("http_recv 0x%08x\n", pcb);

 // memset(Url, 0, HTTP_MAX_REQUEST_LEN);
  hs = arg;

  if (err == ERR_OK && p != NULL) {

    /* Inform TCP that we have taken the data. */
    tcp_recved(pcb, p->tot_len);

    if (hs->recv_state == 2){

	hs->recv_content_len -= p->tot_len;

	if (hs->RecvPbuf == NULL){
		hs->RecvPbuf = p;
		hs->RecvOffset = 0;
	}
	else{
		pbuf_cat(hs->RecvPbuf, p);
	}

       send_data_to_sys(hs);
	   
	   return ERR_OK;
    }

    if (hs->handle == NULL) {
//      data = p->payload;
		if (hs->http_recive_request.Valid){
			temprecive = mem_malloc(hs->http_recive_request.charlen);
			if (NULL == temprecive){
				if (hs->http_recive_request.Httpd_recive_buf){
					mem_free(hs->http_recive_request.Httpd_recive_buf);
					hs->http_recive_request.Httpd_recive_buf = NULL;
				}
				hs->http_recive_request.charlen=0;
				hs->http_recive_request.Valid=0;
				hs->http_recive_request.PreBuf=NULL;
				DEBUG_PRINT("http_recv 00pbuf:0x%x\r\n", (u32)p);
				pbuf_free(p);
		    	close_conn(pcb, hs);
		  		return ERR_OK;	
			}
			if (hs->http_recive_request.Httpd_recive_buf&&hs->http_recive_request.charlen){
				memcpy(temprecive,hs->http_recive_request.Httpd_recive_buf, hs->http_recive_request.charlen);
	    	}
		}

		if (hs->http_recive_request.Httpd_recive_buf){
			mem_free(hs->http_recive_request.Httpd_recive_buf);
			hs->http_recive_request.Httpd_recive_buf = NULL;
		}

		hs->http_recive_request.Httpd_recive_buf=mem_malloc( p->tot_len+hs->http_recive_request.charlen);
		if(hs->http_recive_request.Httpd_recive_buf==NULL)
		{
			DEBUG_PRINT("Httpd Recive Buf Error pbuf:%x\n\r", p);
			hs->http_recive_request.charlen=0;
			hs->http_recive_request.Valid=0;
			hs->http_recive_request.PreBuf=NULL;
			pbuf_free(p);
			close_conn(pcb, hs);
	  		return ERR_OK;		
		}		

		memset(hs->http_recive_request.Httpd_recive_buf,0, p->tot_len+hs->http_recive_request.charlen);
		
		if(hs->http_recive_request.Valid==1)
		{
			if (temprecive){
				memcpy(hs->http_recive_request.Httpd_recive_buf,temprecive,hs->http_recive_request.charlen);
				mem_free(temprecive);
				temprecive = NULL;
			}
		}

		pbuf_copy_partial(p,hs->http_recive_request.Httpd_recive_buf+hs->http_recive_request.charlen, p->tot_len, 0);	
		hs->http_recive_request.charlen +=p->tot_len;
		DEBUG_PRINT("####http_recv pbuf:%x\r\n", p);
		pbuf_free(p);
		hs->http_recive_request.Valid=1;	
		DEBUG_PRINT(hs->http_recive_request.Httpd_recive_buf);
		HtmlLen = 0;
		HtmlBegin = hs->http_recive_request.Httpd_recive_buf;
		do{
			HtmlBegin += HtmlLen;
			if (hs->recv_state != 2){
			    HtmlEnd = strstr(HtmlBegin, "\r\n\r\n"); 
			    if(HtmlEnd){
			             HtmlEnd += 4;
				HtmlLen = (int)(HtmlEnd - HtmlBegin)	;	 
					extract_html_recive(HtmlBegin,hs,pcb);
				hs->http_recive_request.charlen -= HtmlLen;
				if (hs->recv_state == 1){
					hs->recv_content_len -= HtmlLen;	
				}
				
				if (hs->http_recive_request.charlen <= 0){
			  		mem_free(hs->http_recive_request.Httpd_recive_buf);
			  		hs->http_recive_request.charlen=0;
			  		hs->http_recive_request.Httpd_recive_buf=NULL;
			  		hs->http_recive_request.Valid=0;
				}	
		     } 
	     }
	     else{
		HtmlLen = hs->http_recive_request.charlen;
		hs->RecvPbuf = pbuf_alloc(PBUF_TRANSPORT, HtmlLen, PBUF_RAM);
		if (hs->RecvPbuf == NULL){
			DEBUG_PRINT("http_recv recvpBuf\r\n");

			return 0;
		}
		pbuf_take(hs->RecvPbuf, HtmlBegin, HtmlLen);	
		hs->RecvOffset = 0;
		
  		mem_free(hs->http_recive_request.Httpd_recive_buf);
  		hs->http_recive_request.charlen=0;
  		hs->http_recive_request.Httpd_recive_buf=NULL;
  		hs->http_recive_request.Valid=0;

		hs->recv_content_len -= HtmlLen;
	     }	 
	}while((HtmlEnd) && (hs->http_recive_request.charlen > 0));

    } 
	else {
      	pbuf_free(p);
    }
  }

  if (err == ERR_OK && p == NULL) {
    close_conn(pcb, hs);
  }
  return ERR_OK;
}

/*-----------------------------------------------------------------------------------*/
static err_t
http_accept(void *arg, struct tcp_pcb *pcb, err_t err)
{
  struct http_state *hs;

  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(err);

  DEBUG_PRINT("http_accept 0x%08x\n\r", pcb);

  tcp_setprio(pcb, TCP_PRIO_MIN);

  /* Allocate memory for the structure that holds the state of the
     connection. */

  hs = (struct http_state *)mem_malloc(sizeof(struct http_state));

  if (hs == NULL) {
     DEBUG_PRINT("http_accept: Out of memory\n\r"); 
    return ERR_MEM;
  }
 
  if (arg){ /*to be clear accept_pending when used TCP_LISTEN_BACKLOG*/
	  tcp_accepted(arg);
  } 

  /* Initialize the structure. */
  memset(hs,0,sizeof(struct http_state));

  /* Tell TCP that this is the structure we wish to be passed for our
     callbacks. */
  tcp_arg(pcb, hs);
 // hs->pcb = pcb;

  /* Tell TCP that we wish to be informed of incoming data by a call
     to the http_recv() function. */
  tcp_recv(pcb, http_recv);

  tcp_err(pcb, conn_err);

  tcp_poll(pcb, http_poll, 4);
  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
#ifdef INCLUDE_HTTPD_CGI
/*-----------------------------------------------------------------------------------*/
void
http_set_cgi_handlers(const tCGI *pCGIs, int iNumHandlers)
{
    g_pCGIs = pCGIs;
    g_iNumCGIs = iNumHandlers;
}


#endif


static struct tcp_pcb *web_pcb = NULL;


void httpd_init(unsigned short port)
{
    gwebcfgmode = 0;
    if (web_pcb)
    {
    	return;
    }

#ifdef INCLUDE_HTTPD_CGI
  extern tCGI Cgi[8];
  http_set_cgi_handlers(Cgi, sizeof(Cgi)/sizeof(tCGI));
#endif

  web_pcb = tcp_new();
  ip_set_option(web_pcb, SOF_REUSEADDR);
  tcp_bind(web_pcb, IP_ADDR_ANY, port);
  web_pcb = tcp_listen(web_pcb);
  tcp_arg(web_pcb, web_pcb);
  tcp_accept(web_pcb, http_accept);
  //printf("web_pcb:%x\n", web_pcb);
  
}

void httpd_deinit(void)
{
    err_t err;
	if (web_pcb)
	{
		tcp_arg(web_pcb, NULL);
	    if (web_pcb->state == LISTEN)
	    {
		    g_pCGIs = NULL;
		    g_iNumCGIs = 0;
	    	tcp_accept(web_pcb, NULL);
	    }
		else
	    {
		    g_pCGIs = NULL;
		    g_iNumCGIs = 0;

		    tcp_recv(web_pcb, NULL);

		    tcp_accept(web_pcb, NULL);

	        tcp_sent(web_pcb, NULL);

	        tcp_poll(web_pcb, NULL, 4);

	        tcp_err(web_pcb, NULL);

	    }
		err = tcp_close(web_pcb);

		if (err){
			err = tcp_shutdown(web_pcb, 1, 1);
		}
		if (err == 0)
			web_pcb = NULL;
		//printf("httpd_deinit err:%d\n", err);
	}


}



void tls_webserver_init(void)
{   
    httpd_init(80);   
}

void tls_webserver_deinit(void)
{
    httpd_deinit();
}

#ifdef INCLUDE_HTTPD_SSI
/*-----------------------------------------------------------------------------------*/
void
http_set_ssi_handler(tSSIHandler pfnSSIHandler, const char **ppcTags,
                     int iNumTags)
{
    DEBUG_PRINT("http_set_ssi_handler\n\r");

    g_pfnSSIHandler = pfnSSIHandler;
    g_ppcTags = ppcTags;
    g_iNumTags = iNumTags;
}
#endif



/*-----------------------------------------------------------------------------------*/
