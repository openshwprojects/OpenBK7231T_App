#include "wm_type_def.h"
#include "wm_http_client.h"
#include "HTTPClient.h"
#include "HTTPClientWrapper.h"

/*************************************************************************** 
* Function: HTTPClientOpenRequest 
* Description: Allocate memory for a new HTTP Session. 
* 
* Input: Flags: HTTP Session internal API flags, 0 should be passed here. 
* 
* Output: None 
* 
* Return: HTTP Session handle
* 
* Date : 2014-6-6 
****************************************************************************/ 
tls_http_session_handle_t tls_http_client_open_request(tls_http_session_flags_t flags)
{
    return HTTPClientOpenRequest(flags);
}

/*************************************************************************** 
* Function: HTTPClientCloseRequest 
* Description: Closes any active connection and free any used memory. 
* 
* Input: pSession: HTTP Session handle. 
* 
* Output: None 
* 
* Return: HTTP Status 
* 
* Date : 2014-6-6 
****************************************************************************/ 
int tls_http_client_close_request(tls_http_session_handle_t *session)
{
    return HTTPClientCloseRequest(session);
}

#if TLS_CONFIG_HTTP_CLIENT_AUTH
/*************************************************************************** 
* Function: HTTPClientSetAuth 
* Description: Sets the HTTP authentication schema. 
* 
* Input: pSession:       HTTP Session handle. 
*           AuthSchema: HTTP Supported authentication methods.
*           pReserved:    Reserved parameter.
* 
* Output: None 
* 
* Return: HTTP Status 
* 
* Date : 2014-6-6 
****************************************************************************/ 
int tls_http_client_set_auth(tls_http_session_handle_t session,
                             tls_http_auth_achema_t auth_schema,
                             void *reserved)
{
    return HTTPClientSetAuth(session, (HTTP_AUTH_SCHEMA)auth_schema, reserved);
}

/*************************************************************************** 
* Function: HTTPClientSetCredentials 
* Description: Sets credentials for the target host. 
* 
* Input: pSession: HTTP Session handle. 
*           pUserName:   User name.
*           pPassword:    Password.
* 
* Output: None 
* 
* Return: HTTP Status 
* 
* Date : 2014-6-6 
****************************************************************************/ 
int tls_http_client_set_credentials(tls_http_session_handle_t session,
                                    char *user_name, char *password)
{
    return HTTPClientSetCredentials(session, (CHAR *)user_name, (CHAR *)password);
}
#endif

#if TLS_CONFIG_HTTP_CLIENT_PROXY
/*************************************************************************** 
* Function: HTTPClientSetProxy 
* Description: Sets all the proxy related parameters. 
* 
* Input: pSession: HTTP Session handle. 
*           pProxyName: The host name.
*           nPort:            The proxy port number.
*           pUserName:   User name for proxy authentication (can be null).
*           pPassword:    User password for proxy authentication (can be null).
* 
* Output: None 
* 
* Return: HTTP Status 
* 
* Date : 2014-6-6 
****************************************************************************/ 
int tls_http_client_set_proxy(tls_http_session_handle_t session, char *proxy_name,
                              u16 port, char *user_name, char *password)
{
    return HTTPClientSetProxy(session, (CHAR *)proxy_name, port, (CHAR *)user_name, (CHAR *)password);
}
#endif

/*************************************************************************** 
* Function: HTTPClientSetVerb 
* Description: Sets the HTTP verb for the outgoing request. 
* 
* Input: pSession: HTTP Session handle. 
*           HttpVerb: HTTP supported verbs
* 
* Output: None 
* 
* Return: HTTP Status 
* 
* Date : 2014-6-6 
****************************************************************************/ 
int tls_http_client_set_verb(tls_http_session_handle_t session, tls_http_verb_t http_verb)
{
    return HTTPClientSetVerb(session, (HTTP_VERB)http_verb);
}

/*************************************************************************** 
* Function: HTTPClientAddRequestHeaders 
* Description: Add headers to the outgoing request. 
* 
* Input: pSession: HTTP Session handle. 
*           pHeaderName:    The Headers name
*           pHeaderData:      The headers data
*           nInsert:              Reserved could be any
* 
* Output: None 
* 
* Return: HTTP Status 
* 
* Date : 2014-6-6 
****************************************************************************/ 
int tls_http_client_add_request_headers(tls_http_session_handle_t session, char *header_name,
                                        char *header_data, bool insert)
{
    return HTTPClientAddRequestHeaders(session, (CHAR *)header_name, (CHAR *)header_data, (BOOL)insert);
}

/*************************************************************************** 
* Function: HTTPClientSendRequest 
* Description: This function builds the request headers, performs a DNS resolution , 
*                 opens the connection (if it was not opened yet by a previous request or if it has closed) 
*                 and sends the request headers. 
* 
* Input: pSession: HTTP Session handle. 
*           pUrl:               The requested URL
*           pData:             Data to post to the server
*           nDataLength:   Length of posted data
*           TotalLength:     Valid only when http method is post.
*                     TRUE:   Post data to http server.
*                     FALSE: In a post request without knowing the total length in advance so return error or use chunking.
*           nTimeout:        Operation timeout
*           nClientPort:      Client side port 0 for none 
*
* Output: None 
* 
* Return: HTTP Status 
* 
* Date : 2014-6-6 
****************************************************************************/ 
int tls_http_client_send_request(tls_http_session_handle_t session, char *url, void *data,
                                 u32 data_len, bool total_len, u32 time_out, u32 client_port)
{
    return HTTPClientSendRequest(session, (CHAR *)url, data, data_len, (BOOL)total_len, time_out, client_port);
}

/*************************************************************************** 
* Function: HTTPClientWriteData 
* Description: Write data to the remote server. 
* 
* Input: pSession: HTTP Session handle. 
*           pBuffer:             Data to write to the server.
*           nBufferLength:   Length of wtitten data.
*           nTimeout:          Timeout for the operation.
* 
* Output: None 
* 
* Return: HTTP Status 
* 
* Date : 2014-6-6 
****************************************************************************/ 
int tls_http_client_write_data(tls_http_session_handle_t session, void *buffer,
                               u32 buffer_len, u32 time_out)
{
    return HTTPClientWriteData(session, buffer, buffer_len, time_out);
}

/*************************************************************************** 
* Function: HTTPClientRecvResponse 
* Description: Receives the response header on the connection and parses it.
*                Performs any required authentication. 
* 
* Input: pSession: HTTP Session handle. 
*           nTimeout:          Timeout for the operation.
* 
* Output: None 
* 
* Return: HTTP Status 
* 
* Date : 2014-6-6 
****************************************************************************/ 
int tls_http_client_recv_response(tls_http_session_handle_t session, u32 time_out)
{
    return HTTPClientRecvResponse(session, time_out);
}

/*************************************************************************** 
* Function: HTTPClientReadData 
* Description: Read data from the server. Parse out the chunks data.
* 
* Input: pSession: HTTP Session handle. 
*           nBytesToRead:    The size of the buffer (numbers of bytes to read)
*           nTimeout:           Operation timeout in seconds
* 
* Output: pBuffer:              A pointer to a buffer that will be filled with the servers response
*             nBytesRecived:   Count of the bytes that were received in this operation 
* 
* Return: HTTP Status 
* 
* Date : 2014-6-6 
****************************************************************************/ 
int tls_http_client_read_data(tls_http_session_handle_t session, void *buffer,
                              u32 bytes_to_read, u32 time_out, u32 *bytes_recived)
{
    return HTTPClientReadData(session, buffer, bytes_to_read, time_out, bytes_recived);
}

/*************************************************************************** 
* Function: HTTPClientGetInfo 
* Description: Fill the users structure with the session information. 
* 
* Input: pSession: HTTP Session handle. 
* 
* Output: HTTPClient:   The session information. 
* 
* Return: HTTP Status 
* 
* Date : 2014-6-6 
****************************************************************************/ 
int tls_http_client_get_info(tls_http_session_handle_t session, tls_http_client_t *http_client)
{
    return HTTPClientGetInfo(session, http_client);
}

/*************************************************************************** 
* Function: HTTPClientFindFirstHeader 
* Description: Initiate the headr searching functions and find the first header. 
* 
* Input: pSession: HTTP Session handle. 
*           pSearchClue:   Search clue.
* 
* Output: pHeaderBuffer: A pointer to a buffer that will be filled with the header name and value.
*             nLength:          Count of the bytes that were received in this operation.
* 
* Return: HTTP Status 
* 
* Date : 2014-6-6 
****************************************************************************/ 
int tls_http_client_find_first_header(tls_http_session_handle_t session, char *search_clue,
                                      char *header_buffer, u32 *length)
{
    return HTTPClientFindFirstHeader(session, (CHAR *)search_clue, (CHAR *)header_buffer, length);
}

/*************************************************************************** 
* Function: HTTPClientGetNextHeader 
* Description: Find the next header.
* 
* Input: pSession: HTTP Session handle. 
* 
* Output: pHeaderBuffer: A pointer to a buffer that will be filled with the header name and value.
*             nLength:          Count of the bytes that were received in this operation. 
* 
* Return: HTTP Status 
* 
* Date : 2014-6-6 
****************************************************************************/ 
int tls_http_client_get_next_header(tls_http_session_handle_t session,
                                    char *header_buffer, u32 *length)
{
    return HTTPClientGetNextHeader(session, (CHAR *)header_buffer, length);
}

/*************************************************************************** 
* Function: HTTPClientFindCloseHeader 
* Description: Terminate a headers search session. 
* 
* Input: pSession: HTTP Session handle. 
* 
* Output: None 
* 
* Return: HTTP Status 
* 
* Date : 2014-6-6 
****************************************************************************/ 
int tls_http_client_find_close_header(tls_http_session_handle_t session)
{
    return HTTPClientFindCloseHeader(session);
}

