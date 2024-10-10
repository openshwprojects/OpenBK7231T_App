#define _XOPEN_SOURCE 700

#include "lwip/inet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lwip/sockets.h"
#include "lwip/tcp.h"
#include <time.h>
#include "../cmnds/cmd_local.h"
#include "../logging/logging.h"
#include "../new_common.h"



#define MAX_REQUEST_LEN 64	// 64 chars for the request line should be enough
#define TCP_PROTO 6
#ifndef TCP_USER_TIMEOUT
    #define TCP_USER_TIMEOUT 18  // how long for loss retry before timeout [ms]
#endif
#define DEFAULT_HOST "192.168.0.1"
// http takes some time until result is ready to process - so add an amount of seconds to the time received in header 
#define DEFAULT_OFFSET 2	 
static int offset=DEFAULT_OFFSET;	
char request[MAX_REQUEST_LEN];

err_t myrecv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
    char outbuff[100];
    char tempout[1024];
    char gmt[4];
    gmt[3]='\0';
    char date[36];
    date[35]='\0';
    int found=0;
 if (p != NULL)
  {
  pbuf_copy_partial(p, tempout, p->tot_len, 0); 
//ADDLOG_INFO(LOG_FEATURE_CMD, " get_HTTP_Header / myreecv -- recv total %d  this buffer %d next %d err %d\nContent:%s", p->tot_len, p->len, p->next, err, tempout);
    
// this is what we will get as date line in header
//  Date: Fri, 01 Mar 2024 18:40:30 GMT
// get pointer to start of "Date:" line
	u16_t start = pbuf_memfind(p,"\r\nDate:",7,0);
	if (start == 0xFFFF ) {
		fprintf(stderr, "No Date found !!\n");
		pbuf_free(p);
	 	tcp_close(pcb);
		return(EXIT_FAILURE);		
	}
	else {		// if (start == 0xFFFF)  --> Date found ...
	// simple check: we know that start points to the beginning of "Date:" line
	// we know it's length and that it ends with "GMT"
	int conv = pbuf_copy_partial(p, date,35, start+2); 	
	gmt[0]=date[32];gmt[1]=date[33]; gmt[2]=date[34]; 
	if (conv < 35  || strstr(date, "GMT") - date != 32 ){
		ADDLOG_ERROR(LOG_FEATURE_CMD, "  get_HTTP_Header -- Date:-line found but format is unknown");
		fprintf(stderr, "Date line found, but unknown format [%s] - conv=%d -- gmt=%s - found at %i (values strstr(date, 'GMT')=%i  AND date=%i)\n",date,conv,gmt,strstr(date, "GMT")-date,strstr(date, "GMT"),date);
		pbuf_free(p);
 		tcp_close(pcb);
		return(EXIT_FAILURE);		
	}
	else { // if (conv != 2 || strcmp(gmt, "GMT") )
	struct tm mytime;
	memset (&mytime,0,sizeof(struct tm));
	// we eliminated the "GMT" from the end of the "Date:" line
	// Date: Sat, 02 Mar 2024 14:21:51
	/*
	W800 doesn't know strptime - so let's do parsing by sscanf 
	strptime(date,"Date: %a, %d %b %Y %H:%M:%S",&mytime);
	*/
	static const char * const monname[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	char month[4];

	sscanf(date, "Date: %*[^,], %d %3s %d %d:%d:%d", &mytime.tm_mday, month, &mytime.tm_year, &mytime.tm_hour, &mytime.tm_min, &mytime.tm_sec);
	mytime.tm_year -= 1900;
//	bk_printf("\r\n\r\nAfter sscanf - Date: mday=%d month=%3s year=%d Hour=%d Min=%d Sec=%d\r\n", mytime.tm_mday, month, mytime.tm_year, mytime.tm_hour, mytime.tm_min, mytime.tm_sec);
	
	for (int i = 0; i < 12; i++) {
		if (strncmp (month, monname[i], 3) == 0) {
		    mytime.tm_mon = i;
		    break;
		}
	}

	strftime(outbuff,sizeof(outbuff),"%d %b %Y %H:%M:%S -- %s", &mytime);

	ADDLOG_INFO(LOG_FEATURE_CMD, "  get_HTTP_Header -- output=%s",outbuff);
	g_epochOnStartup = mktime(&mytime) - g_secondsElapsed + offset;
	ADDLOG_INFO(LOG_FEATURE_CMD, "  get_HTTP_Header -- Date found:%s -- calculated (GMT) epoch on startup:%i",date,g_epochOnStartup);

//	g_UTCoffset = 3600;
	found=1;    
     tcp_recved(pcb, p->tot_len);
   }  // else { // if (conv != 2 || strcmp(gmt, "GMT") )
  }//else {		// if (! start)  --> Date found ...
  }
 else {  // if (p != NULL)
//	 ADDLOG_INFO(LOG_FEATURE_CMD, " get_HTTP_Header -- in myreecv -- p == NULL");
};
if (p) pbuf_free(p); 
tcp_close(pcb);	
 return ERR_OK;
}


err_t connected(void *arg, struct tcp_pcb *pcb, err_t err)
{
int sendbuff = tcp_sndbuf(pcb);
 if (sendbuff){
       do {
	err = tcp_write(pcb, request, strlen(request), 0);
// 	 ADDLOG_INFO(LOG_FEATURE_CMD, " get_HTTP_Header -- in connected after write -- error=%i",err);
//         bk_printf("\r\nerr: %d", err);

        } while (err == ERR_MEM);


	err = tcp_output(pcb);
 //	 ADDLOG_INFO(LOG_FEATURE_CMD, " get_HTTP_Header -- in connected after output -- error=%i",err);
 }
 else{
 	 ADDLOG_ERROR(LOG_FEATURE_CMD, " get_HTTP_Header -- error getting buffer space (sendbuff=%i)",sendbuff);
 	 
 }
 
  tcp_arg(pcb, NULL);
  tcp_sent(pcb, NULL);

return ERR_OK;
}


int get_HTTP_header(const char* hostname, unsigned short server_port) {
    int request_len;
    struct sockaddr_in sockaddr_in;
    
// we only need the header to get date and time
    request_len = snprintf(request, MAX_REQUEST_LEN, "HEAD / HTTP/1.1\r\nHost: %s\r\nConnection: Close\r\n\r\n", hostname);   
    if (request_len >= MAX_REQUEST_LEN) {
        fprintf(stderr, "request length too large: %d\n", request_len);
        exit(EXIT_FAILURE);
    }

    struct tcp_pcb *pcb = tcp_new();
    tcp_recv(pcb, myrecv);
    ip_addr_t ip;
    ipaddr_aton(hostname,&ip);

    err_t err = tcp_connect(pcb, &ip, server_port, connected);
    tcp_recv(pcb, myrecv);
 
  return 1;
 }


// Can be called w/o any argument --> get header from actual gateway with default port and default offset
// or called with argument(s)	IP [HTTP port] [offset in secs] 
static commandResult_t CMD_GET_HTTP_Headertime(const void* context, const char* cmd, const char* args, int cmdFlags) {

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_ALLOW_ESCAPING_QUOTATIONS);
//ADDLOG_INFO(LOG_FEATURE_CMD, "  Tokenizer_GetArgsCount()=%i --  Tokenizer_GetArg(0)=%s -- Tokenizer_GetArgIntegerDefault(1, 80)=%i ",Tokenizer_GetArgsCount(),Tokenizer_GetArg(0),Tokenizer_GetArgIntegerDefault(1, 80));
	offset = Tokenizer_GetArgIntegerDefault(2, DEFAULT_OFFSET);
	unsigned short port = (unsigned short)Tokenizer_GetArgIntegerDefault(1, 80);
	ADDLOG_INFO(LOG_FEATURE_CMD, " CMD_GET_HTTP_Headertime%s%s - IP:%s port:%i Offset:%i",Tokenizer_GetArgsCount()==0 ? "" : " - args: ", args,Tokenizer_GetArgsCount()==0 ? HAL_GetMyGatewayString() : Tokenizer_GetArg(0), port, offset);

	get_HTTP_header(Tokenizer_GetArgsCount()==0 ? HAL_GetMyGatewayString() : Tokenizer_GetArg(0), port);
//	get_HTTP_header();
	return CMD_RES_OK;
}

int CMD_InitGetHeaderTime() {
	CMD_RegisterCommand("getHeaderTime", CMD_GET_HTTP_Headertime, NULL);
}

