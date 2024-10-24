#include <string.h>
#include "wm_include.h"
#include "wm_http_fwup.h"

#if DEMO_HTTP
extern int t_http_fwup(char *url);
#define    HTTP_CLIENT_BUFFER_SIZE   1024

u8 RemoteIp[4] = {192, 168, 1, 100};	//Remote server's IP when test http function

u32 http_snd_req(HTTPParameters ClientParams, HTTP_VERB verb, char *pSndData, u8 parseXmlJson)
{
    int                   nRetCode;
    u32                   nSize, nTotal = 0;
    char                 *Buffer = NULL;
    HTTP_SESSION_HANDLE   pHTTP;
    u32                   nSndDataLen ;
    do
    {
        Buffer = (char *)tls_mem_alloc(HTTP_CLIENT_BUFFER_SIZE);
        if(Buffer == NULL)
        {
            return HTTP_CLIENT_ERROR_NO_MEMORY;
        }
        memset(Buffer, 0, HTTP_CLIENT_BUFFER_SIZE);
        printf("HTTP Client v1.0\r\n");
        nSndDataLen = (pSndData == NULL ? 0 : strlen(pSndData));
        // Open the HTTP request handle
        pHTTP = HTTPClientOpenRequest(0);
        if(!pHTTP)
        {
            nRetCode =  HTTP_CLIENT_ERROR_INVALID_HANDLE;
            break;
        }
        // Set the Verb
        nRetCode = HTTPClientSetVerb(pHTTP, verb);
        if(nRetCode != HTTP_CLIENT_SUCCESS)
        {
            break;
        }
#if TLS_CONFIG_HTTP_CLIENT_AUTH
        // Set authentication
        if(ClientParams.AuthType != AuthSchemaNone)
        {
            if((nRetCode = HTTPClientSetAuth(pHTTP, ClientParams.AuthType, NULL)) != HTTP_CLIENT_SUCCESS)
            {
                break;
            }

            // Set authentication
            if((nRetCode = HTTPClientSetCredentials(pHTTP, ClientParams.UserName, ClientParams.Password)) != HTTP_CLIENT_SUCCESS)
            {
                break;
            }
        }
#endif //TLS_CONFIG_HTTP_CLIENT_AUTH
#if TLS_CONFIG_HTTP_CLIENT_PROXY
        // Use Proxy server
        if(ClientParams.UseProxy == TRUE)
        {
            if((nRetCode = HTTPClientSetProxy(pHTTP, ClientParams.ProxyHost, ClientParams.ProxyPort, NULL, NULL)) != HTTP_CLIENT_SUCCESS)
            {

                break;
            }
        }
#endif //TLS_CONFIG_HTTP_CLIENT_PROXY
        if((nRetCode = HTTPClientSendRequest(pHTTP, ClientParams.Uri, pSndData, nSndDataLen, verb == VerbPost || verb == VerbPut, 0, 0)) != HTTP_CLIENT_SUCCESS)
        {
            break;
        }
        // Retrieve the the headers and analyze them
        if((nRetCode = HTTPClientRecvResponse(pHTTP, 30)) != HTTP_CLIENT_SUCCESS)
        {
            break;
        }
        printf("Start to receive data from remote server...\r\n");

        // Get the data until we get an error or end of stream code
        while(nRetCode == HTTP_CLIENT_SUCCESS || nRetCode != HTTP_CLIENT_EOS)
        {
            // Set the size of our buffer
            nSize = HTTP_CLIENT_BUFFER_SIZE;
            // Get the data
            nRetCode = HTTPClientReadData(pHTTP, Buffer, nSize, 300, &nSize);
            if(nRetCode != HTTP_CLIENT_SUCCESS && nRetCode != HTTP_CLIENT_EOS)
                break;
            printf("%s", Buffer);
            nTotal += nSize;
        }
    }
    while(0);   // Run only once
    tls_mem_free(Buffer);

    if(pHTTP)
        HTTPClientCloseRequest(&pHTTP);
    if(ClientParams.Verbose == TRUE)
    {
        printf("\n\nHTTP Client terminated %d (got %d b)\n\n", nRetCode, nTotal);
    }
    return nRetCode;
}

u32 http_get(HTTPParameters ClientParams)
{
    return http_snd_req(ClientParams, VerbGet, NULL, 0);
}

u32 http_post(HTTPParameters ClientParams, char *pSndData)
{
    return http_snd_req(ClientParams, VerbPost, pSndData, 0);
}

u32 http_put(HTTPParameters ClientParams, char *pSndData)
{
    return http_snd_req(ClientParams, VerbPut, pSndData, 0);
}


int http_get_demo(char *buf)
{
    HTTPParameters httpParams;

    memset(&httpParams, 0, sizeof(HTTPParameters));
    httpParams.Uri = (char *)tls_mem_alloc(128);
    if(httpParams.Uri == NULL)
    {
        printf("malloc error.\n");
        return WM_FAILED;
    }
    memset(httpParams.Uri, 0, 128);
    sprintf(httpParams.Uri, "%s", buf);
    httpParams.Verbose = TRUE;
    printf("Location: %s\n", httpParams.Uri);
    http_get(httpParams);
    tls_mem_free(httpParams.Uri);

    return WM_SUCCESS;
}
int http_post_demo(char *postData)
{
    HTTPParameters httpParams;

    memset(&httpParams, 0, sizeof(HTTPParameters));
    httpParams.Uri = (char *)tls_mem_alloc(128);
    if(httpParams.Uri == NULL)
    {
        printf("malloc error.\n");
        return WM_FAILED;
    }
    memset(httpParams.Uri, 0, 128);
    sprintf(httpParams.Uri, "http://%d.%d.%d.%d:8080/TestWeb/login.do", RemoteIp[0], RemoteIp[1], RemoteIp[2], RemoteIp[3]);
    printf("Location: %s\n", httpParams.Uri);
    httpParams.Verbose = TRUE;
    http_post(httpParams, postData);
    tls_mem_free(httpParams.Uri);
    return WM_SUCCESS;
}
int http_put_demo(char *putData)
{
    HTTPParameters httpParams;

    memset(&httpParams, 0, sizeof(HTTPParameters));
    httpParams.Uri = (char *)tls_mem_alloc(128);
    if(httpParams.Uri == NULL)
    {
        printf("malloc error.\n");
        return WM_FAILED;
    }
    memset(httpParams.Uri, 0, 128);
    sprintf(httpParams.Uri, "http://%d.%d.%d.%d:8080/TestWeb/login_put.do", RemoteIp[0], RemoteIp[1], RemoteIp[2], RemoteIp[3]);
    printf("Location: %s\n", httpParams.Uri);
    httpParams.Verbose = TRUE;
    http_put(httpParams, putData);
    tls_mem_free(httpParams.Uri);
    return WM_SUCCESS;
}

int http_fwup_demo(char *url)
{
    t_http_fwup(url);

    return WM_SUCCESS;
}

#endif //DEMO_HTTP

