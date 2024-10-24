#include <string.h>
#include "wm_include.h"
#include "wm_config.h"
#include "wm_http_client.h"
#include "lwip/sys.h"
#include "wm_wl_task.h"

#if TLS_CONFIG_HTTP_CLIENT_TASK
#define HTTP_CLIENT_STK_SIZE 1024
static sys_mbox_t http_client_mbox = SYS_MBOX_NULL;
static u32 *httpClientStk = NULL;
static tls_os_task_t httpClientTaskHandle = NULL;
#define    HTTP_CLIENT_BUFFER_SIZE   1024
extern u8 pSession_flag;
static UINT32  http_snd_req_local(
	HTTP_SESSION_HANDLE pHTTP, HTTPParameters ClientParams, HTTP_VERB verb, CHAR* pSndData, u32 dataLen,
	http_client_recv_callback_fn recv_fn)
{
		INT32                   nRetCode;
    UINT32                  nSize,nTotal = 0;
    CHAR*                   Buffer = NULL;
    UINT32                  nSndDataLen ;
    CHAR token[32];
    UINT32 size=32;
    char *pRecvData=NULL;
    u32 totallen;
    do
    {
        Buffer = (CHAR*)tls_mem_alloc(HTTP_CLIENT_BUFFER_SIZE);
        if(Buffer == NULL)
        {
            return HTTP_CLIENT_ERROR_NO_MEMORY;
        }
        memset(Buffer, 0, HTTP_CLIENT_BUFFER_SIZE);
        //printf("\nHTTP Client v1.0\n\n");
        nSndDataLen = (pSndData==NULL ? 0 : dataLen);
        // Set the Verb
        nRetCode = HTTPClientSetVerb(pHTTP,verb);
        if(nRetCode != HTTP_CLIENT_SUCCESS)
        {
            break;
        }
#if TLS_CONFIG_HTTP_CLIENT_AUTH
        // Set authentication
        if(ClientParams.AuthType != AuthSchemaNone)
        {
            if((nRetCode = HTTPClientSetAuth(pHTTP,ClientParams.AuthType,NULL)) != HTTP_CLIENT_SUCCESS)
            {
                break;
            }

            // Set authentication
            if((nRetCode = HTTPClientSetCredentials(pHTTP,ClientParams.UserName,ClientParams.Password)) != HTTP_CLIENT_SUCCESS)
            {
                break;
            }
        }
#endif //TLS_CONFIG_HTTP_CLIENT_AUTH
#if TLS_CONFIG_HTTP_CLIENT_PROXY
        // Use Proxy server
        if(ClientParams.UseProxy == TRUE)
        {
            if((nRetCode = HTTPClientSetProxy(pHTTP,ClientParams.ProxyHost,ClientParams.ProxyPort,NULL,NULL)) != HTTP_CLIENT_SUCCESS)
            {

                break;
            }
        }
#endif //TLS_CONFIG_HTTP_CLIENT_PROXY
	 if((nRetCode = HTTPClientSendRequest(pHTTP,ClientParams.Uri,pSndData,nSndDataLen,verb==VerbPost || verb==VerbPut,0,0)) != HTTP_CLIENT_SUCCESS)
        {
            break;
        }
        // Retrieve the the headers and analyze them
        if((nRetCode = HTTPClientRecvResponse(pHTTP,30)) != HTTP_CLIENT_SUCCESS)
        {
            break;
        }
        memset(token, 0, 32);
        if( ((nRetCode = HTTPClientFindFirstHeader(pHTTP, "Content-Length", token, &size)) != HTTP_CLIENT_SUCCESS) &&
		  ((nRetCode = HTTPClientFindFirstHeader(pHTTP, "content-length", token, &size)) != HTTP_CLIENT_SUCCESS) )
        {
        	totallen = HTTP_CLIENT_BUFFER_SIZE;
        }
        else
        {
        	totallen = atol(strstr(token,":")+1);
        	
        }
        HTTPClientFindCloseHeader(pHTTP);
	 //printf("Start to receive data from remote server...\n");
        // Get the data until we get an error or end of stream code
        pSession_flag = 0;
        while(nRetCode == HTTP_CLIENT_SUCCESS || nRetCode != HTTP_CLIENT_EOS)
        {
            // Set the size of our buffer
            nSize = HTTP_CLIENT_BUFFER_SIZE;   
            // Get the data
            nRetCode = HTTPClientReadData(pHTTP,Buffer,nSize,300,&nSize);
            if(nRetCode != HTTP_CLIENT_SUCCESS && nRetCode != HTTP_CLIENT_EOS)
                break;
            pRecvData = tls_mem_alloc(nSize);
            if(pRecvData == NULL)
            {
                nRetCode = HTTP_CLIENT_ERROR_NO_MEMORY;
                break;
            }
            memcpy(pRecvData, Buffer, nSize);
            recv_fn(pHTTP, pRecvData, totallen, nSize);
        }
    } while(0); // Run only once
    if(Buffer)
        tls_mem_free(Buffer);
    if(pHTTP)
        HTTPClientCloseRequest(&pHTTP);
    if(ClientParams.Verbose == TRUE)
    {
        printf("\n\nHTTP Client terminated %d (got %d kb)\n\n",nRetCode,(nTotal/ 1024));
    }
    return nRetCode;
}

static void http_client_rx(void *sdata)
{
	int ret;
	void *msg;
	http_client_msg *http_msg;
	for(;;) 
	{
		sys_arch_mbox_fetch(&http_client_mbox, (void **)&msg, 0);
		http_msg = (http_client_msg *)msg;
		ret = http_snd_req_local(http_msg->pSession,
					http_msg->param,
					http_msg->method,
					http_msg->sendData,
					http_msg->dataLen,
					http_msg->recv_fn);
		//printf("http_client_rx ret=%d\n", ret);
		if(ret == HTTP_CLIENT_SUCCESS || ret == HTTP_CLIENT_EOS)
		{
            ;
		}
		else
		{
			if(http_msg->err_fn != NULL)
				http_msg->err_fn(http_msg->pSession, ret);
		}
		if(http_msg->sendData != NULL)
			tls_mem_free(http_msg->sendData);
		if(http_msg->param.Uri)
			tls_mem_free(http_msg->param.Uri);
		tls_mem_free(http_msg);
	}

}


int http_client_task_init(void)
{
	tls_os_status_t err = TLS_OS_SUCCESS;
	if (http_client_mbox)
	{
		return WM_SUCCESS;
	}

	if(sys_mbox_new(&http_client_mbox, 32) != ERR_OK) 
	{
          return WM_FAILED;
    }

	httpClientStk = (u32 *)tls_mem_alloc(HTTP_CLIENT_STK_SIZE * sizeof(u32));
	if (httpClientStk)
	{
		err = tls_os_task_create(&httpClientTaskHandle, "httpc",
				http_client_rx,
	                    NULL,
	                    (void *)httpClientStk,              /* task's stack start address */
	                    HTTP_CLIENT_STK_SIZE * sizeof(u32), /* task's stack size, unit:byte */
	                    TLS_HTTP_CLIENT_TASK_PRIO,
	                    0);
		if (err != TLS_OS_SUCCESS)
		{
			sys_mbox_free(&http_client_mbox);
			tls_mem_free(httpClientStk);
			httpClientStk = NULL;
		}
	}
	else
	{
		sys_mbox_free(&http_client_mbox);	
	}
	return WM_SUCCESS;
}

void http_client_task_free(void)
{
	if (httpClientStk)
	{
		tls_mem_free(httpClientStk);
		httpClientStk = NULL;
		if (http_client_mbox)
		{
			sys_mbox_free(&http_client_mbox);
			http_client_mbox = SYS_MBOX_NULL;
		}
	}
}

int http_client_task_deinit(void)
{
	if (httpClientTaskHandle)
	{
		tls_os_task_del_by_task_handle(httpClientTaskHandle,http_client_task_free);
	}
	return 0;
}


int http_client_post(http_client_msg * msg)
{
	int len;
	err_t err;
	http_client_msg * msg1 = tls_mem_alloc(sizeof(http_client_msg));
	if(!msg1)
		return HTTP_CLIENT_ERROR_NO_MEMORY;
	memset(msg1, 0, sizeof(http_client_msg));
	memcpy(msg1, msg, sizeof(http_client_msg));
	if(msg->dataLen > 0)
	{
		msg1->sendData = tls_mem_alloc(msg->dataLen);
		if(msg1->sendData == NULL)
		{
			tls_mem_free(msg1);
			return HTTP_CLIENT_ERROR_NO_MEMORY;
		}
		memcpy(msg1->sendData, msg->sendData, msg->dataLen);
	}
	else
		msg1->sendData = NULL;
	if(msg->param.Uri != NULL)
	{
		len = strlen(msg->param.Uri);
		msg1->param.Uri = tls_mem_alloc(len + 1);
		if(msg1->param.Uri == NULL)
		{
			if(msg1->sendData)
				tls_mem_free(msg1->sendData);
	        tls_mem_free(msg1);
			return HTTP_CLIENT_ERROR_NO_MEMORY;
		}
		memset(msg1->param.Uri, 0, len + 1);
		memcpy(msg1->param.Uri,msg->param.Uri,len);
	}
	
        // Open the HTTP request handle
        msg->pSession= HTTPClientOpenRequest(0);
        if(!msg->pSession)
        {
        	if(msg1->sendData)
			tls_mem_free(msg1->sendData);
		if(msg1->param.Uri)
			tls_mem_free(msg1->param.Uri);
        	tls_mem_free(msg1);
		return HTTP_CLIENT_ERROR_INVALID_HANDLE;
    }
	msg1->pSession = msg->pSession;
	if (WM_SUCCESS == http_client_task_init())
	{
		err = sys_mbox_trypost(&http_client_mbox, msg1);
	}
	else
	{
		err = ERR_MEM;
	}
    //printf("post err=%d\n", err);
	if(err != ERR_OK)
	{
		HTTPClientCloseRequest(&msg->pSession);
        	if(msg1->sendData)
			tls_mem_free(msg1->sendData);
		if(msg1->param.Uri)
			tls_mem_free(msg1->param.Uri);
		tls_mem_free(msg1);
	}
	return err;
}


#endif //TLS_CONFIG_HTTP_CLIENT_TASK

