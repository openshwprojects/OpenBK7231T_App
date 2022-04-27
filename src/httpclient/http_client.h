/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#ifndef _UTILS_HTTPC_H_
#define _UTILS_HTTPC_H_

#include "utils_net.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup HttpClient
 * @{
 * HttpClient API implements the client-side of HTTP/1.1. It provides base interfaces to execute an HTTP request on a given URL. It also supports HTTPS (HTTP over TLS) to provide secure communication.\n
 * @section HttpClient_Usage_Chapter How to use this module
 * In this release, MediaTek provides two types of APIs: high level APIs and low level APIs.\n
 * - \b The \b high \b level \b APIs
 *  - Enables to execute a single HTTP request on a given URL.
 *  - Call #httpclient_get(), #httpclient_post(), #httpclient_put() or #httpclient_delete() to get, post, put or delete and HTTP request.\n
 * - \b The \b low \b level \b APIs
 *  - Enables to execute more than one HTTP requests during a Keep-Alive connection. Keep-alive is the idea of using a single TCP connection to send and receive multiple HTTP requests/responses, as opposed to opening a new connection for every single request/response pair.
 *  - Step1: Call #httpclient_connect() to connect to a remote server.
 *  - Step2: Call #httpclient_send_request() to send an HTTP request to the server.
 *  - Step3: Call #httpclient_recv_response() to receive an HTTP response from the server.
 *  - Step4: Repeat Steps 2 and 3 to execute more requests.
 *  - Step5: Call #httpclient_close() to close the connection.
 *  - Sample code: Please refer to the example under <sdk_root>/project/mt7687_hdk/apps/http_client/http_client_keepalive folder.
 */

/** @defgroup httpclient_define Define
 * @{
 */
/** @brief   This macro defines the HTTP port.  */
#define HTTP_PORT   80

/** @brief   This macro defines the HTTPS port.  */
#define HTTPS_PORT 443
/**
 * @}
 */
#define TCP_LEN_MAX             1460

/** @defgroup httpclient_enum Enum
 *  @{
 */
/** @brief   This enumeration defines the HTTP request type.  */
typedef enum {
    HTTPCLIENT_GET,
    HTTPCLIENT_POST,
    HTTPCLIENT_PUT,
    HTTPCLIENT_DELETE,
    HTTPCLIENT_HEAD
} HTTPCLIENT_REQUEST_TYPE;

/** @defgroup httpclient_struct Struct
 * @{
 */
/** @brief   This structure defines the httpclient_t structure.  */
typedef struct {
    //intptr_t handle; /**< handle of TCP connection or SSL connection. */
    int remote_port; /**< HTTP or HTTPS port. */
    utils_network_t net;
    int response_code; /**< Response code. */
    const char *header; /**< Custom header. */
    char *auth_user; /**< Username for basic authentication. */
    char *auth_password; /**< Password for basic authentication. */
} httpclient_t;

/** @brief   This structure defines the HTTP data structure.  */
typedef struct {
    bool is_more; /**< Indicates if more data needs to be retrieved. */
    bool is_chunked; /**< Response data is encoded in portions/chunks.*/
    int retrieve_len; /**< Content length to be retrieved (total). */
    uint32_t response_content_len; /**< Response content length. (total)*/
    uint32_t post_buf_len; /**< Post data length. */
    uint32_t response_buf_len; /**< Response buffer length. */
    char *post_content_type; /**< Content type of the post data. */
    char *post_buf; /**< User data to be posted. */
    char *response_buf; /**< Buffer to store the response data. */
    uint32_t response_buf_filled; /** how much real data in response_buff */
} httpclient_data_t;

// should the library call free( ) on request struct when done?
#define HTTPREQUEST_FLAG_FREE_SELFONDONE 1
// should the library call free( ) on url when done?
#define HTTPREQUEST_FLAG_FREE_URLONDONE 2

typedef struct httprequest_t_tag{
    int state;
    int (*data_callback)(struct httprequest_t_tag *request);
    httpclient_t client;
    const char *url;
    const char *header;
    int port;
    short method;
	// eg. HTTPREQUEST_FLAG_FREE_SELFONDONE
    short flags;
    const char *ca_crt;
    uint32_t timeout;
    httpclient_data_t client_data;
    void *usercontext; // anything you like
} httprequest_t;


/**
 * @brief            This function executes a request on a given URL. It returns immediately and calls back with state and data.
 * @param[in]        request is a pointer to the #httprequest_t.
 * @return           .
 * @par              HTTPClient_Async_SendGeneric Post Example
 * @code
 *                   httprequest_t *request = os_malloc(sizeof(httprequest_t));
 *                   // NOTE: these MUST persist
 *                   char *url = "https://api.mediatek.com/mcs/v2/devices/D0n2yhrl/datapoints.csv";
 *                   char *header = "deviceKey:FZoo0S07CpwUHcrt\r\n";
 *                   char *content_type = "text/csv";
 *                   char *post_data = "1,,I am string!";
 *                   httpclient_t *client = request.client;
 *                   httpclient_data_t *client_data = request.client_data;
 *                   char *buf = NULL;
 *                   buf = pvPortMalloc(BUF_SIZE);
 *                   if (buf == NULL) {
 *                       printf("Malloc failed.\r\n");
 *                       return;
 *                   }
 *                   memset(buf, 0, sizeof(buf));
 *                   client_data->.response_buf = buf;  //Sets a buffer to store the result.
 *                   client_data->response_buf_len = BUF_SIZE;  //Sets the buffer size.
 *                   HTTPClient_SetCustomHeader(&client, header);  //Sets the custom header if needed.
 *                   client_data->post_buf = post_data;  //Sets the user data to be posted.
 *                   client_data->post_buf_len = strlen(post_data);  //Sets the post data length.
 *                   client_data->post_content_type = content_type;  //Sets the content type.
 *                   request->port = HTTPS_PORT;
 *                   request->url = url;
 *                   request->method = HTTPCLIENT_POST; 
 *                   request->timeout = 10000;
 *                   request->data_callback = mydatacallback; 
 *                   HTTPClient_Async_SendGeneric(request);
 * 
 * 
 * int mydatacallback(httprequest_t *request){
 *      addLog("httpclient callback state %d", request->state);
 * 
 *      //to terminate, return 1
 *      return 0
 * }
 * @endcode
 */
int HTTPClient_Async_SendGeneric(httprequest_t *request);
int HTTPClient_Async_SendGet(const char *url_in);
void HTTPClient_SetCustomHeader(httpclient_t *client, const char *header);

#ifdef __cplusplus
}
#endif

#endif /* __HTTPCLIENT_H__ */

