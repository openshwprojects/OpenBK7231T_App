#include <string.h>
#include <stdio.h>
#include "wm_socket_fwup.h"
#include "wm_fwup.h"
#include "wm_http_fwup.h"
#include "wm_debug.h"
#include "wm_mem.h"

#if TLS_CONFIG_HTTP_CLIENT

#define HTTP_CLIENT_BUFFER_SIZE  2048
#define FWUP_MSG_SIZE			 3
#define USE_BREAK_POINT_RESUME   1
#define RECV_TIMEOUT             5



enum ota_state {PREPARE_PACKET, SETUP_LINK_AND_REQ, RECV_RSP, HANDLE_HEADER, HANDLE_BODY, SHUTDOWN_LINK, QUIT_OTA};

int http_fwup(HTTPParameters ClientParams)
{
    char* Buffer = NULL;  
    char token[32];
    char headRange[32] = {0};
    int  nRetCode = 0;
    u32  content_length=0, size=32;
    u32  partLen;
    u32 totalLen = 0;
    u32 recvLen = 0;
    IMAGE_HEADER_PARAM_ST *booter;
#if USE_BREAK_POINT_RESUME == 0
    u32 breakFlag = 0;
    u32 breakLen = 0;
#endif
    struct pbuf *p;
    HTTP_SESSION_HANDLE  pHTTP;
    enum ota_state now_state = PREPARE_PACKET;

    Buffer = (char*)tls_mem_alloc(HTTP_CLIENT_BUFFER_SIZE + FWUP_MSG_SIZE);
    if (!Buffer)
        return HTTP_CLIENT_ERROR_NO_MEMORY;
    memset(Buffer, 0, HTTP_CLIENT_BUFFER_SIZE + FWUP_MSG_SIZE);    

    while(1)
    {
        switch(now_state)
        {
            case PREPARE_PACKET:
            {
                memset(token, 0, 32);
                size = 32;
                pHTTP = HTTPClientOpenRequest(0);
#if USE_BREAK_POINT_RESUME
                HTTPClientSetVerb(pHTTP,VerbGet);
                sprintf(headRange, "bytes=%d-", recvLen);
                if((nRetCode = HTTPClientAddRequestHeaders(pHTTP,"Range", headRange, 1))!= HTTP_CLIENT_SUCCESS){
                    now_state = QUIT_OTA;
                }
#else
                if( recvLen != 0 ){
                    breakFlag = 1;
                    breakLen = 0;
                }
#endif
                now_state = SETUP_LINK_AND_REQ;
            }
            break;
            
            case SETUP_LINK_AND_REQ:
            {
                if( (nRetCode = HTTPClientSendRequest(pHTTP,ClientParams.Uri,NULL,0,FALSE,0,0)) != HTTP_CLIENT_SUCCESS ){
                    tls_os_time_delay(HZ*2);
                    now_state = SHUTDOWN_LINK;
                }
                else
                    now_state = RECV_RSP;
            }
            break;
            
            case RECV_RSP:
            {
                if((nRetCode = HTTPClientRecvResponse(pHTTP, RECV_TIMEOUT)) != HTTP_CLIENT_SUCCESS)
                    now_state = SHUTDOWN_LINK;
                else
                    now_state = HANDLE_HEADER;
            }
            break;
            
            case HANDLE_HEADER:
            {
                if((nRetCode = HTTPClientFindFirstHeader(pHTTP, "content-length", (CHAR *)token, &size)) != HTTP_CLIENT_SUCCESS){
                    HTTPClientFindCloseHeader(pHTTP);
                    now_state = SHUTDOWN_LINK;
                }
                else
                {
                    HTTPClientFindCloseHeader(pHTTP);
                    content_length = atol(strstr(token,":")+1);
                    printf("content_length: %d\n", content_length);
                    now_state = HANDLE_BODY;
                    if(recvLen == 0){
                        nRetCode = socket_fwup_accept(0, ERR_OK);
                        if(nRetCode != ERR_OK)
                            now_state = QUIT_OTA;
                    }
                }
            }
            break;
            
            case HANDLE_BODY:
            {
                partLen = 0;
                while(nRetCode != HTTP_CLIENT_EOS )
                {
                    u32 nSize = HTTP_CLIENT_BUFFER_SIZE;
                    nRetCode = HTTPClientReadData(pHTTP,Buffer+FWUP_MSG_SIZE,nSize,RECV_TIMEOUT,&nSize);
                    if( recvLen == 0 ){
             
                        booter =(IMAGE_HEADER_PARAM_ST *) (Buffer+FWUP_MSG_SIZE);
						if (TRUE == tls_fwup_img_header_check(booter))
						{
							totalLen = booter->img_len + sizeof(IMAGE_HEADER_PARAM_ST);
							if (booter->img_attr.b.signature)
							{
								totalLen += 128;
							}
						}
						else
						{
							now_state = QUIT_OTA;
							break;
						}                        
                    }

                    if(nRetCode != HTTP_CLIENT_SUCCESS && nRetCode != HTTP_CLIENT_EOS){
                        now_state = SHUTDOWN_LINK;
                        break;
                    }
#if USE_BREAK_POINT_RESUME == 0
                    if(breakFlag == 1){
                        breakLen += nSize;
                        if(breakLen <= recvLen){
                            continue;
                        }
                        else{
                            Buffer = Buffer+FWUP_MSG_SIZE+nSize-(breakLen-recvLen)-FWUP_MSG_SIZE;
                            nSize = (breakLen-recvLen);
                            breakFlag = 0;
                        }
                    }
#endif

                    p = pbuf_alloc(PBUF_TRANSPORT, nSize + FWUP_MSG_SIZE, PBUF_REF);
                    while( !p){
                        tls_os_time_delay(1);
                        p = pbuf_alloc(PBUF_TRANSPORT, nSize + FWUP_MSG_SIZE, PBUF_REF);
                    }
                    
                    if(recvLen == 0)
                        *(Buffer+0) = SOCKET_FWUP_START;
                    else if(nRetCode == HTTP_CLIENT_EOS && recvLen == (totalLen-nSize))
                    {
                        recvLen += nSize;
                        partLen += nSize;           
						if (totalLen && (recvLen == totalLen))
                        printf("download %d / %d\n", recvLen, totalLen);
                        *(Buffer+0) = SOCKET_FWUP_END;
                    }
                    else
                    {
                        *(Buffer+0) = SOCKET_FWUP_DATA;
                    }

                    *(Buffer+1) = (nSize>>8) & 0xFF;
                    *(Buffer+2) = nSize & 0xFF;	
                    p->payload =  Buffer;
                    p->len = p->tot_len = nSize + FWUP_MSG_SIZE;
                    // Send received data to fwup thread and deal with socket issues.
                    s8 ret = socket_fwup_recv(0, p, ERR_OK);
                    if(ret != ERR_OK){
						if (ret == ERR_VAL)
						{
                        	now_state = QUIT_OTA;
						}
						else
						{
							now_state = SHUTDOWN_LINK;
						}
                        break;
                    }
                    else{
                        recvLen += nSize;
                        partLen += nSize;           
                        printf("download %d / %d\n", recvLen, totalLen);
                        if(partLen == content_length){
                            now_state = SHUTDOWN_LINK;
                            break;
                        }
                    }
                }
				if (now_state == QUIT_OTA);
				else
                now_state = SHUTDOWN_LINK;
            }
            break;
            
            case SHUTDOWN_LINK:
            {
                if(pHTTP){
                    HTTPClientCloseRequest(&pHTTP);
                    (recvLen == totalLen)?(now_state = QUIT_OTA):(now_state = PREPARE_PACKET);
                }
            }
            break;
            
            default:
            break;
        }
        //printf("now_state %d\n", now_state);
        if(now_state == QUIT_OTA)
            break;
    }

    tls_mem_free(Buffer);
    if(pHTTP)
        HTTPClientCloseRequest(&pHTTP);
    if(ClientParams.Verbose == TRUE)
    {
        printf("\n\nHTTP Client terminated %d (got %d kb)\n\n",nRetCode,(recvLen/ 1024));
    }
    if(nRetCode)
        socket_fwup_err(0, nRetCode);
    return nRetCode;
}

int t_http_fwup(char *url)
{
	HTTPParameters httpParams;
	memset(&httpParams, 0, sizeof(HTTPParameters));
	if (url == NULL)
	{
		httpParams.Uri = "http://192.168.1.100:8080/w800_ota.img";
	}
	else
	{
		httpParams.Uri = url;
	}

	printf("Location: %s\n",httpParams.Uri);
	httpParams.Verbose = TRUE;
	return http_fwup(httpParams);
}

#endif //TLS_CONFIG_HTTP_CLIENT && TLS_CONFIG_SOCKET_RAW

