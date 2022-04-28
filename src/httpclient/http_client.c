// intent: implement http client code
// should have a callback to supply data from the HTTP request to the caller in chunks.
// should indicate when the request ends

/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#include "../new_common.h"
#include "include.h"
#include "utils_timer.h"
//#include "lite-log.h"
#include "http_client.h"
#include "rtos_pub.h"
#include "../logging/logging.h"

#include "iot_export_errno.h"

#define log_err(a, ...)
#define log_debug(a, ...)
#define log_info(a, ...)
#define log_warning(a, ...)


#define HTTPCLIENT_MIN(x,y) (((x)<(y))?(x):(y))
#define HTTPCLIENT_MAX(x,y) (((x)>(y))?(x):(y))

#define HTTPCLIENT_AUTHB_SIZE     128

#define HTTPCLIENT_CHUNK_SIZE     256
//#define HTTPCLIENT_SEND_BUF_SIZE  2048
#define HTTPCLIENT_SEND_BUF_SIZE  256

// even github release link is 84 chars..
#define HTTPCLIENT_MAX_HOST_LEN   128
//#define HTTPCLIENT_MAX_URL_LEN    2048

#define HTTP_RETRIEVE_MORE_DATA   (1)            /**< More data needs to be retrieved. */



static int httpclient_parse_host(const char *url, char *host, int *port, uint32_t maxhost_len);
static int httpclient_parse_url(const char *url, char *scheme, uint32_t max_scheme_len, char *host,
                                uint32_t maxhost_len, int *port, char *path, uint32_t max_path_len);
static int httpclient_conn(httpclient_t *client);
static int httpclient_recv(httpclient_t *client, char *buf, int min_len, int max_len, int *p_read_len,
                           uint32_t timeout);
static int httpclient_retrieve_content(httpclient_t *client, char *data, int len, uint32_t timeout,
                                       httpclient_data_t *client_data);
static int httpclient_response_parse(httpclient_t *client, char *data, int len, uint32_t timeout,
                                     httpclient_data_t *client_data);

static void httpclient_base64enc(char *out, const char *in)
{
    const char code[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
    int i = 0, x = 0, l = 0;

    for (; *in; in++) {
        x = x << 8 | *in;
        for (l += 8; l >= 6; l -= 6) {
            out[i++] = code[(x >> (l - 6)) & 0x3f];
        }
    }
    if (l > 0) {
        x <<= 6 - l;
        out[i++] = code[x & 0x3f];
    }
    for (; i % 4;) {
        out[i++] = '=';
    }
    out[i] = '\0';
}

int httpclient_conn(httpclient_t *client)
{
	int ret;
	ret = client->net.doConnect(&client->net);
    if (0 != ret) {
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "establish connection failed, error %i ",ret);
        return ERROR_HTTP_CONN;
    }

    return SUCCESS_RETURN;
}

int httpclient_parse_url(const char *url, char *scheme, uint32_t max_scheme_len, char *host, uint32_t maxhost_len,
                         int *port, char *path, uint32_t max_path_len)
{
    char *scheme_ptr = (char *) url;
    char *host_ptr = (char *) os_strstr(url, "://");
    uint32_t host_len = 0;
    uint32_t path_len;
    //char *port_ptr;
    char *path_ptr;
    char *fragment_ptr;

    if (host_ptr == NULL) {
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "Could not find host");
        return ERROR_HTTP_PARSE; /* URL is invalid */
    }

    if (max_scheme_len < host_ptr - scheme_ptr + 1) {
        /* including NULL-terminating char */
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "Scheme str is too small (%u >= %u)", max_scheme_len, (uint32_t)(host_ptr - scheme_ptr + 1));
        return ERROR_HTTP_PARSE;
    }
    os_memcpy(scheme, scheme_ptr, host_ptr - scheme_ptr);
    scheme[host_ptr - scheme_ptr] = '\0';

    host_ptr += 3;

    *port = 0;

    path_ptr = os_strchr(host_ptr, '/');
    if (NULL == path_ptr) {
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "invalid path");
        return -1;
    }

    if (host_len == 0) {
        host_len = path_ptr - host_ptr;
    }

    if (maxhost_len < host_len + 1) {
        /* including NULL-terminating char */
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "Host str is too long (host_len(%d) >= max_len(%d))", host_len + 1, maxhost_len);
        return ERROR_HTTP_PARSE;
    }
    os_memcpy(host, host_ptr, host_len);
    host[host_len] = '\0';

    fragment_ptr = os_strchr(host_ptr, '#');
    if (fragment_ptr != NULL) {
        path_len = fragment_ptr - path_ptr;
    } else {
        path_len = os_strlen(path_ptr);
    }

    if (max_path_len < path_len + 1) {
        /* including NULL-terminating char */
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "Path str is too small (%d >= %d)", max_path_len, path_len + 1);
        return ERROR_HTTP_PARSE;
    }
    os_memcpy(path, path_ptr, path_len);
    path[path_len] = '\0';

    return SUCCESS_RETURN;
}

int httpclient_parse_host(const char *url, char *host, int *port, uint32_t maxhost_len)
{
    const char *host_ptr = (const char *) os_strstr(url, "://");
    uint32_t host_len = 0;
    char *path_ptr;

    ADDLOG_INFO(LOG_FEATURE_HTTP_CLIENT, "Parse url %s\r\n", url);

    if (!strncmp(url, "HTTPS://", 8)){
        ADDLOG_INFO(LOG_FEATURE_HTTP_CLIENT, "HTTPS:// found -> port 443\r\n");
        *port = 443;
    } else {
        if (!strncmp(url, "HTTP://", 7)){
            ADDLOG_INFO(LOG_FEATURE_HTTP_CLIENT, "HTTP:// found -> port 80\r\n");
            *port = 80;
        }
    }

    if (host_ptr == NULL) {
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "Could not find host");
        return ERROR_HTTP_PARSE; /* URL is invalid */
    }
    host_ptr += 3;

    path_ptr = os_strchr(host_ptr, '/');
    host_len = path_ptr - host_ptr;

    if (maxhost_len < host_len + 1) {
        /* including NULL-terminating char */
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "Host str is too small (%d >= %d)", maxhost_len, host_len + 1);
        return ERROR_HTTP_PARSE;
    }
    os_memcpy(host, host_ptr, host_len);
    host[host_len] = '\0';

    // parse port from host, and truncate host if found.
    char *port_ptr = os_strchr(host, ':');
    if (port_ptr){
        *port_ptr = 0;
        port_ptr++;
        int p = 0;
        int num = sscanf(port_ptr, "%d", &p);
        if (num == 1){
            *port = p;
        }
        ADDLOG_INFO(LOG_FEATURE_HTTP_CLIENT, "Host split into %s port %d\r\n", host, *port);
    }

    return SUCCESS_RETURN;
}

int httpclient_get_info(httpclient_t *client, char *send_buf, int *send_idx, char *buf,
                        uint32_t len) /* 0 on success, err code on failure */
{
    int ret;
    int cp_len;
    int idx = *send_idx;

    if (len == 0) {
        len = os_strlen(buf);
    }

    do {
        if ((HTTPCLIENT_SEND_BUF_SIZE - idx) >= len) {
            cp_len = len;
        } else {
            cp_len = HTTPCLIENT_SEND_BUF_SIZE - idx;
        }

        os_memcpy(send_buf + idx, buf, cp_len);
        idx += cp_len;
        len -= cp_len;

        if (idx == HTTPCLIENT_SEND_BUF_SIZE) {
            //            if (client->remote_port == HTTPS_PORT)
            //            {
            //                WRITE_IOT_ERROR_LOG("send buffer overflow");
            //                return ERROR_HTTP;
            //            }
            //ret = httpclient_tcp_send_all(client->handle, send_buf, HTTPCLIENT_SEND_BUF_SIZE);
            ret = client->net.doWrite(&client->net, send_buf, HTTPCLIENT_SEND_BUF_SIZE, 5000);
            if (ret) {
                return (ret);
            }
        }
    } while (len);

    *send_idx = idx;
    return SUCCESS_RETURN;
}

void HTTPClient_SetCustomHeader(httpclient_t *client, const char *header)
{
    client->header = header;
}

int httpclient_basic_auth(httpclient_t *client, char *user, char *password)
{
    if ((os_strlen(user) + os_strlen(password)) >= HTTPCLIENT_AUTHB_SIZE) {
        return ERROR_HTTP;
    }
    client->auth_user = user;
    client->auth_password = password;
    return SUCCESS_RETURN;
}

int httpclient_send_auth(httpclient_t *client, char *send_buf, int *send_idx)
{
    char b_auth[(int)((HTTPCLIENT_AUTHB_SIZE + 3) * 4 / 3 + 1)];
    char base64buff[HTTPCLIENT_AUTHB_SIZE + 3];

    httpclient_get_info(client, send_buf, send_idx, "Authorization: Basic ", 0);
    sprintf(base64buff, "%s:%s", client->auth_user, client->auth_password);
    ADDLOG_DEBUG(LOG_FEATURE_HTTP_CLIENT, "bAuth: %s", base64buff) ;
    httpclient_base64enc(b_auth, base64buff);
    b_auth[os_strlen(b_auth) + 1] = '\0';
    b_auth[os_strlen(b_auth)] = '\n';
    ADDLOG_DEBUG(LOG_FEATURE_HTTP_CLIENT, "b_auth:%s", b_auth) ;
    httpclient_get_info(client, send_buf, send_idx, b_auth, 0);
    return SUCCESS_RETURN;
}

int httpclient_send_header(httpclient_t *client, const char *url, int method, httpclient_data_t *client_data)
{
    char scheme[8] = { 0 };
    //char host[HTTPCLIENT_MAX_HOST_LEN] = { 0 };
    //char path[HTTPCLIENT_MAX_URL_LEN] = { 0 };
    char *host;//[HTTPCLIENT_MAX_HOST_LEN] = { 0 };
    char *path;//[HTTPCLIENT_MAX_URL_LEN] = { 0 };
    int len;
    //char send_buf[HTTPCLIENT_SEND_BUF_SIZE] = { 0 };
    //char buf[HTTPCLIENT_SEND_BUF_SIZE] = { 0 };
    char *send_buf;
    char *buf;
    char *meth = (method == HTTPCLIENT_GET) ? "GET" : (method == HTTPCLIENT_POST) ? "POST" :
                 (method == HTTPCLIENT_PUT) ? "PUT" : (method == HTTPCLIENT_DELETE) ? "DELETE" :
                 (method == HTTPCLIENT_HEAD) ? "HEAD" : "";
    int ret;
    int port;
    int rc = SUCCESS_RETURN;

    if (NULL == (host = (char *)os_malloc(HTTPCLIENT_MAX_HOST_LEN))) {
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "not enough memory");
        return FAIL_RETURN;
    }
    if (NULL == (path = (char *)os_malloc(HTTPCLIENT_MAX_HOST_LEN))) {
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "not enough memory");
        rc = FAIL_RETURN;
        goto GO_ERR_3;
    }
    if (NULL == (send_buf = (char *)os_malloc(HTTPCLIENT_SEND_BUF_SIZE))) {
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "not enough memory");
        rc = FAIL_RETURN;
        goto GO_ERR_2;
    }
    if (NULL == (buf = (char *)os_malloc(HTTPCLIENT_SEND_BUF_SIZE))) {
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "not enough memory");
        rc = FAIL_RETURN;
        goto GO_ERR_1;
    }
    /* First we need to parse the url (http[s]://host[:port][/[path]]) */
    int res = httpclient_parse_url(url, scheme, sizeof(scheme), host, HTTPCLIENT_MAX_HOST_LEN, &port, path, HTTPCLIENT_MAX_HOST_LEN);
    if (res != SUCCESS_RETURN) {
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "httpclient_parse_url returned %d\r\n", res);
        //return res;
        rc = res;
        goto GO_ERR;
    }

    //if (client->remote_port == 0)
    //{
    if (os_strcmp(scheme, "http") == 0) {
        //client->remote_port = HTTP_PORT;
    } else if (os_strcmp(scheme, "https") == 0) {
        //client->remote_port = HTTPS_PORT;
    }
    //}

    /* Send request */
    os_memset(send_buf, 0, HTTPCLIENT_SEND_BUF_SIZE);
    os_memset(buf, 0, HTTPCLIENT_SEND_BUF_SIZE);
    len = 0; /* Reset send buffer */

    snprintf(buf, HTTPCLIENT_SEND_BUF_SIZE, "%s %s HTTP/1.1\r\nHost: %s\r\n", meth, path, host); /* Write request */
    ret = httpclient_get_info(client, send_buf, &len, buf, os_strlen(buf));
    if (ret) {
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "Could not write request");
        //return ERROR_HTTP_CONN;
        rc = ERROR_HTTP_CONN;
        goto GO_ERR;
    }

    /* Send all headers */
    if (client->auth_user) {
        httpclient_send_auth(client, send_buf, &len); /* send out Basic Auth header */
    }

    /* Add user header information */
    if (client->header) {
        httpclient_get_info(client, send_buf, &len, (char *) client->header, os_strlen(client->header));
    }

    if (client_data->post_buf != NULL) {
        snprintf(buf, HTTPCLIENT_SEND_BUF_SIZE, "Content-Length: %u\r\n", (unsigned int)client_data->post_buf_len);
        httpclient_get_info(client, send_buf, &len, buf, os_strlen(buf));

        if (client_data->post_content_type != NULL) {
            snprintf(buf, HTTPCLIENT_SEND_BUF_SIZE, "Content-Type: %s\r\n", client_data->post_content_type);
            httpclient_get_info(client, send_buf, &len, buf, os_strlen(buf));
        }
    }

    /* Close headers */
    httpclient_get_info(client, send_buf, &len, "\r\n", 0);

    //log_multi_line(LOG_DEBUG_LEVEL, "REQUEST", "%s", send_buf, ">");

    //ret = httpclient_tcp_send_all(client->net.handle, send_buf, len);
    ret = client->net.doWrite(&client->net, send_buf, len, 5000);
    if (ret > 0) {
        ADDLOG_DEBUG(LOG_FEATURE_HTTP_CLIENT, "Written %d bytes\r\n", ret);
    } else if (ret == 0) {
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "ret == 0,Connection was closed by server\r\n");
        //return ERROR_HTTP_CLOSED; /* Connection was closed by server */
        rc = ERROR_HTTP_CLOSED;
    } else {
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "Connection error (send returned %d)\r\n", ret);
        //return ERROR_HTTP_CONN;
        rc = ERROR_HTTP_CONN;
    }
GO_ERR:
    os_free(buf);
GO_ERR_1:
    os_free(send_buf);
GO_ERR_2:
    os_free(path);
GO_ERR_3:
    os_free(host);
    return rc;//SUCCESS_RETURN;
}

int httpclient_send_userdata(httpclient_t *client, httpclient_data_t *client_data)
{
    int ret = 0;

    if (client_data->post_buf && client_data->post_buf_len) {
        ADDLOG_DEBUG(LOG_FEATURE_HTTP_CLIENT, "client_data->post_buf: %s", client_data->post_buf);
        {
            //ret = httpclient_tcp_send_all(client->handle, (char *)client_data->post_buf, client_data->post_buf_len);
            ret = client->net.doWrite(&client->net, (char *)client_data->post_buf, client_data->post_buf_len, 5000);
            if (ret > 0) {
                ADDLOG_DEBUG(LOG_FEATURE_HTTP_CLIENT, "Written %d bytes", ret);
            } else if (ret == 0) {
                ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "ret == 0,Connection was closed by server");
                return ERROR_HTTP_CLOSED; /* Connection was closed by server */
            } else {
                ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "Connection error (send returned %d)", ret);
                return ERROR_HTTP_CONN;
            }
        }
    }

    return SUCCESS_RETURN;
}

/* 0 on success, err code on failure */
// reads into buf up to max_len.
// put len in *p_read_len
int httpclient_recv(httpclient_t *client, char *buf, int min_len, int max_len, int *p_read_len, uint32_t timeout_ms)
{
    int ret = 0;
    iotx_time_t timer;

    iotx_time_init(&timer);
    utils_time_countdown_ms(&timer, timeout_ms);

    *p_read_len = 0;

    ret = client->net.doRead(&client->net, buf, max_len, iotx_time_left(&timer));
    //log_debug("Recv: | %s", buf);

    if (ret > 0) {
        *p_read_len = ret;
    } else if (ret == 0) {
        //timeout
        return 0;
    } else if (-1 == ret) {
        ADDLOG_INFO(LOG_FEATURE_HTTP_CLIENT, "Connection closed.\r\n");
        return ERROR_HTTP_CONN;
    } else {
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "Connection error (recv returned %d)\r\n", ret);
        return ERROR_HTTP_CONN;
    }
    ADDLOG_INFO(LOG_FEATURE_HTTP_CLIENT, "httpclient_recv %u bytes has been read\r\n", *p_read_len);
    return 0;
}



// called with data and len from header parser.
// called with len = 0 when getting more
int httpclient_retrieve_content(httpclient_t *client, char *data, int len, uint32_t timeout_ms,
                                httpclient_data_t *client_data)
{
    //int count = 0;
    //int templen = 0;
    //int crlf_pos;
    //iotx_time_t timer;
    //char * b_data = NULL;

    //iotx_time_init(&timer);
    //utils_time_countdown_ms(&timer, timeout_ms);

    /* Receive data */
    ADDLOG_DEBUG(LOG_FEATURE_HTTP_CLIENT, "Current data len: %d\r\n", len);

    client_data->is_more = true;

    memcpy(client_data->response_buf, data, len);
    client_data->response_buf_filled = len;
    client_data->retrieve_len -= len;

    if (client_data->retrieve_len <= 0){
        client_data->is_more = false;
    }

    return SUCCESS_RETURN;
}



#ifdef INCLUDE_OLD_FUNCTION
// called with data and len from header parser.
// called with len = 0 when getting more
int httpclient_retrieve_content_old(httpclient_t *client, char *data, int len, uint32_t timeout_ms,
                                httpclient_data_t *client_data)
{
    int count = 0;
    int templen = 0;
    int crlf_pos;
    iotx_time_t timer;
    char * b_data = NULL;

    iotx_time_init(&timer);
    utils_time_countdown_ms(&timer, timeout_ms);

    /* Receive data */
    log_debug("Current data: %s\r\n", data);

    client_data->is_more = true;

    if (client_data->response_content_len == -1 && client_data->is_chunked == false) {
        log_debug("httpclient_retrieve_content client_data->response_content_len == -1 && client_data->is_chunked == false\r\n");
        while (true) {
            int ret, max_len;
            if (count + len < client_data->response_buf_len - 1) {
                os_memcpy(client_data->response_buf + count, data, len);
                count += len;
                client_data->response_buf[count] = '\0';
                client_data->response_buf_filled = count;
            } else {
                os_memcpy(client_data->response_buf + count, data, client_data->response_buf_len - 1 - count);
                client_data->response_buf[client_data->response_buf_len - 1] = '\0';
                client_data->response_buf_filled = client_data->response_buf_len - 1;
                return HTTP_RETRIEVE_MORE_DATA;
            }

            max_len = HTTPCLIENT_MIN(HTTPCLIENT_CHUNK_SIZE - 1, client_data->response_buf_len - 1 - count);
            ret = httpclient_recv(client, data, 1, max_len, &len, iotx_time_left(&timer));

            /* Receive data */
            log_debug("rxed data len: %d %d\r\n", len, count);

            if (ret == ERROR_HTTP_CONN) {
                log_debug("ret == ERROR_HTTP_CONN");
                return ret;
            }

            if (len == 0) {
                /* read no more data */
                log_debug("no more len == 0\r\n");
                client_data->is_more = false;
                return SUCCESS_RETURN;
            }
        }
        // there is no break above, only returns, so this will never happen.
        return SUCCESS_RETURN;
    }

    log_debug("httpclient_retrieve_content client_data->response_content_len == %d && client_data->is_chunked == %d\r\n", client_data->response_content_len, client_data->is_chunked);


    while (true) {
        uint32_t readLen = 0;

        if (client_data->is_chunked && client_data->retrieve_len <= 0) {
            log_debug("httpclient_retrieve_content Read chunk header client_data->retrieve_len == %d && client_data->is_chunked == %d\r\n", client_data->retrieve_len, client_data->is_chunked);
            /* Read chunk header */
            bool foundCrlf;
            int n;
            do {
                foundCrlf = false;
                crlf_pos = 0;
                data[len] = 0;
                if (len >= 2) {
                    for (; crlf_pos < len - 2; crlf_pos++) {
                        if (data[crlf_pos] == '\r' && data[crlf_pos + 1] == '\n') {
                            foundCrlf = true;
                            break;
                        }
                    }
                }
                if (!foundCrlf) {
                    /* Try to read more */
                    if (len < HTTPCLIENT_CHUNK_SIZE) {
                        int new_trf_len, ret;
                        ret = httpclient_recv(client,
                                              data + len,
                                              0,
                                              HTTPCLIENT_CHUNK_SIZE - len - 1,
                                              &new_trf_len,
                                              iotx_time_left(&timer));
                        len += new_trf_len;
                        if (ret == ERROR_HTTP_CONN) {
                            return ret;
                        } else {
                            continue;
                        }
                    } else {
                        return ERROR_HTTP;
                    }
                }
            } while (!foundCrlf);
            data[crlf_pos] = '\0';
            n = sscanf(data, "%x", &readLen);/* chunk length */
            client_data->retrieve_len = readLen;
            client_data->response_content_len += client_data->retrieve_len;
            if (n != 1) {
                log_err("Could not read chunk length");
                return ERROR_HTTP_UNRESOLVED_DNS;
            }

            os_memmove(data, &data[crlf_pos + 2], len - (crlf_pos + 2)); /* Not need to move NULL-terminating char any more */
            len -= (crlf_pos + 2);

            if (readLen == 0) {
                /* Last chunk */
                client_data->is_more = false;
                log_debug("no more (last chunk)");
                break;
            }
        } else {
            readLen = client_data->retrieve_len;
        }

        log_debug("Total-Payload: %d Bytes; Read: %d Bytes\r\n", readLen, len);
        #if HTTP_WR_TO_FLASH
        http_flash_init();
        http_wr_to_flash(data,len);
        #endif

        
        b_data =  os_malloc((TCP_LEN_MAX+1) * sizeof(char));
        //bk_http_ptr->do_data = 1;
        //bk_http_ptr->http_total = readLen - len;
        do {
            templen = HTTPCLIENT_MIN(len, readLen);
            if (count + templen < client_data->response_buf_len - 1) {
                count += templen;
                log_debug("Copy data %d bytes to output %d\r\n", templen);
                os_memcpy(client_data->response_buf+count, data, templen);
                client_data->response_buf_filled = count;
                client_data->response_buf[count] = '\0';
                client_data->retrieve_len -= templen;                
            } else {
                client_data->response_buf[client_data->response_buf_len - 1] = '\0';
                client_data->response_buf_filled = client_data->response_buf_len - 1;
                client_data->retrieve_len -= (client_data->response_buf_len - 1 - count);
                os_free(b_data);
                b_data = NULL;
                return HTTP_RETRIEVE_MORE_DATA;
            }

            if (len > readLen) {
                log_debug("memmove %d %d %d\r\n", readLen, len, client_data->retrieve_len);
                os_memmove(b_data, &b_data[readLen], len - readLen); /* chunk case, read between two chunks */
                len -= readLen;
                readLen = 0;
                client_data->retrieve_len = 0;
            } else {
                readLen -= len;
            }

            if (readLen) {
                int ret;
                int max_len = HTTPCLIENT_MIN(TCP_LEN_MAX, client_data->response_buf_len - 1 - count);
                max_len = HTTPCLIENT_MIN(max_len, readLen);

                ret = httpclient_recv(client, b_data, 1, max_len, &len, iotx_time_left(&timer));
                if (ret == ERROR_HTTP_CONN) {
                    os_free(b_data);
                    b_data = NULL;
                    return ret;
                }
            }
        } while (readLen);
        
        //bk_http_ptr->do_data = 0;
        os_free(b_data);
        b_data = NULL;
            
        if (client_data->is_chunked) {
            if (len < 2) {
                int new_trf_len, ret;
                /* Read missing chars to find end of chunk */
                ret = httpclient_recv(client, data + len, 2 - len, HTTPCLIENT_CHUNK_SIZE - len - 1, &new_trf_len,
                                      iotx_time_left(&timer));
                if (ret == ERROR_HTTP_CONN) {
                    return ret;
                }
                len += new_trf_len;
            }
            if ((data[0] != '\r') || (data[1] != '\n')) {
                log_err("Format error, %s", data); /* after memmove, the beginning of next chunk */
                return ERROR_HTTP_UNRESOLVED_DNS;
            }
            os_memmove(data, &data[2], len - 2); /* remove the \r\n */
            len -= 2;
        } else {
            log_debug("no more (content-length)\r\n");
            client_data->is_more = false;
            break;
        }

    }

    return SUCCESS_RETURN;
}
#endif


int httpclient_response_parse(httpclient_t *client, char *data, int len, uint32_t timeout_ms,
                              httpclient_data_t *client_data)
{
    int crlf_pos;
    iotx_time_t timer;

    iotx_time_init(&timer);
    utils_time_countdown_ms(&timer, timeout_ms);

    client_data->response_content_len = -1;

    char *crlf_ptr = os_strstr(data, "\r\n");
    if (crlf_ptr == NULL) {
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "\r\n not found");
        return ERROR_HTTP_UNRESOLVED_DNS;
    }

    crlf_pos = crlf_ptr - data;
    data[crlf_pos] = '\0';

    /* Parse HTTP response */
    if (sscanf(data, "HTTP/%*d.%*d %d %*[^\r\n]", &(client->response_code)) != 1) {
        /* Cannot match string, error */
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "Not a correct HTTP answer : %s\r\n", data);
        return ERROR_HTTP_UNRESOLVED_DNS;
    }

    if ((client->response_code < 200) || (client->response_code >= 400)) {
        /* Did not return a 2xx code; TODO fetch headers/(&data?) anyway and implement a mean of writing/reading headers */
        ADDLOG_WARN(LOG_FEATURE_HTTP_CLIENT, "Response code %d\r\n", client->response_code);
    }

    ADDLOG_DEBUG(LOG_FEATURE_HTTP_CLIENT, "Reading headers%s\r\n", data);

    os_memmove(data, &data[crlf_pos + 2], len - (crlf_pos + 2) + 1); /* Be sure to move NULL-terminating char as well */
    len -= (crlf_pos + 2);

    client_data->is_chunked = false;

    /* Now get headers */
    while (true) {
        char key[32];
        char value[32];
        int n;

        key[31] = '\0';
        value[31] = '\0';

        crlf_ptr = os_strstr(data, "\r\n");
        if (crlf_ptr == NULL) {
            if (len < HTTPCLIENT_CHUNK_SIZE - 1) {
                int new_trf_len, ret;
                ret = httpclient_recv(client, data + len, 1, HTTPCLIENT_CHUNK_SIZE - len - 1, &new_trf_len, iotx_time_left(&timer));
                len += new_trf_len;
                data[len] = '\0';
                ADDLOG_DEBUG(LOG_FEATURE_HTTP_CLIENT, "Read %d chars; In buf: [%s]\r\n", new_trf_len, data);
                if (ret == ERROR_HTTP_CONN) {
                    return ret;
                } else {
                    continue;
                }
            } else {
                ADDLOG_DEBUG(LOG_FEATURE_HTTP_CLIENT, "header len > chunksize\r\n");
                return ERROR_HTTP;
            }
        }

        crlf_pos = crlf_ptr - data;
        if (crlf_pos == 0) {
            /* End of headers */
            os_memmove(data, &data[2], len - 2 + 1); /* Be sure to move NULL-terminating char as well */
            len -= 2;
            break;
        }

        data[crlf_pos] = '\0';

        n = sscanf(data, "%31[^:]: %31[^\r\n]", key, value);
        if (n == 2) {
            ADDLOG_DEBUG(LOG_FEATURE_HTTP_CLIENT, "Read header : %s: %s\r\n", key, value);
            if (!os_strcmp(key, "Content-Length")) {
                sscanf(value, "%d", (int *)&(client_data->response_content_len));
                client_data->retrieve_len = client_data->response_content_len;
            } else if (!os_strcmp(key, "Transfer-Encoding")) {
                if (!os_strcmp(value, "Chunked") || !os_strcmp(value, "chunked")) {
                    client_data->is_chunked = true;
                    client_data->response_content_len = 0;
                    client_data->retrieve_len = 0;
                }
            }
            os_memmove(data, &data[crlf_pos + 2], len - (crlf_pos + 2) + 1); /* Be sure to move NULL-terminating char as well */
            len -= (crlf_pos + 2);

        } else {
            ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "Could not parse header\r\n");
            return ERROR_HTTP;
        }
    }

    if(client->response_code != 200)
        {
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "Could not found\r\n");
        return MQTT_SUB_INFO_NOT_FOUND_ERROR;
    }

    return httpclient_retrieve_content(client, data, len, iotx_time_left(&timer), client_data);
}

int httpclient_connect(httpclient_t *client)
{
    int ret = ERROR_HTTP_CONN;

    client->net.handle = 0;

    ret = httpclient_conn(client);
    //    if (0 == ret)
    //    {
    //        client->remote_port = HTTP_PORT;
    //    }

    return ret;
}

iotx_err_t httpclient_send_request(httpclient_t *client, const char *url, int method, httpclient_data_t *client_data)
{
    int ret = ERROR_HTTP_CONN;

    if (0 == client->net.handle) {
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "not connection have been established");
        return ret;
    }

    ret = httpclient_send_header(client, url, method, client_data);
    if (ret != 0) {
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "httpclient_send_header is error,ret = %d", ret);
        return ret;
    }

    if (method == HTTPCLIENT_POST || method == HTTPCLIENT_PUT) {
        ret = httpclient_send_userdata(client, client_data);
    }

    return ret;
}

iotx_err_t httpclient_recv_response(httpclient_t *client, uint32_t timeout_ms, httpclient_data_t *client_data)
{
    int reclen = 0, ret = ERROR_HTTP_CONN;
    char buf[HTTPCLIENT_CHUNK_SIZE] = { 0 };
    iotx_time_t timer;

    iotx_time_init(&timer);
    utils_time_countdown_ms(&timer, timeout_ms);

    if (0 == client->net.handle) {
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "not connection have been established");
        return ret;
    }

    int max_len = client_data->response_buf_len;
    int lentoget = HTTPCLIENT_CHUNK_SIZE - 1;
    if (lentoget > max_len){
        lentoget = max_len;
    }

    // first time it's called, client_data->is_more is false.
    // and this causes the headers to be parsed until data is reached
    if (!client_data->is_more) {
        client_data->is_more = 1;
        ret = httpclient_recv(client, buf, 1, lentoget, &reclen, iotx_time_left(&timer));
        if (ret != 0) {
            return ret;
        }

        buf[reclen] = '\0';

        if (reclen) {
            //log_multi_line(LOG_DEBUG_LEVEL, "RESPONSE", "%s", buf, "<");
            ret = httpclient_response_parse(client, buf, reclen, iotx_time_left(&timer), client_data);
        }
        return ret;
    }


    // if is_more is true, we just need to get more data
    // this will fill response_buf 
    if (client_data->is_more) {
        client_data->response_buf[0] = '\0';
        client_data->response_buf_filled = 0;
        reclen = 0; // no existing data
        ret = httpclient_recv(client, buf, 1, lentoget, &reclen, iotx_time_left(&timer));
        if (ret != 0) {
            return ret;
        }
        ret = httpclient_retrieve_content(client, 
            buf, 
            reclen, 
            iotx_time_left(&timer), client_data);
    }

    return ret;
}

void httpclient_close(httpclient_t *client)
{
    if (client->net.handle > 0) {
        client->net.doDisconnect(&client->net);
    }
    client->net.handle = 0;
}

void httpclient_freeMemory(httprequest_t *request)
{
	if(request->flags & HTTPREQUEST_FLAG_FREE_URLONDONE) {
		free((void*)request->url);
	}
	if(request->flags & HTTPREQUEST_FLAG_FREE_SELFONDONE) {
		free((void*)request);
	}
}
int httpclient_common(httpclient_t *client, const char *url, int port, const char *ca_crt, int method,
                      uint32_t timeout_ms,
                      httpclient_data_t *client_data)
{
    iotx_time_t timer;
    int ret = 0;
    char host[HTTPCLIENT_MAX_HOST_LEN] = { 0 };

    if (0 == client->net.handle) {
        //Establish connection if no.
    	httpclient_parse_host(url, host, &port, sizeof(host));
    	ADDLOG_DEBUG(LOG_FEATURE_HTTP_CLIENT, "host: '%s', port: %d\r\n", host, port);

    	iotx_net_init(&client->net, host, port, ca_crt);

    	ret = httpclient_connect(client);
    	if (0 != ret) {
            ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "httpclient_connect is error,ret = %d\r\n", ret);
            httpclient_close(client);
            return ret;
    	}

        ret = httpclient_send_request(client, url, method, client_data);
        if (0 != ret) {
            ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "httpclient_send_request is error,ret = %d\r\n", ret);
            httpclient_close(client);
            return ret;
        }
    }

    iotx_time_init(&timer);
    utils_time_countdown_ms(&timer, timeout_ms);

     if ((NULL != client_data->response_buf)
         || (0 != client_data->response_buf_len)) {
        ret = httpclient_recv_response(client, iotx_time_left(&timer), client_data);
        if (ret < 0) {
            ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "httpclient_recv_response is error,ret = %d\r\n", ret);
            httpclient_close(client);
            return ret;
        }
    }

    if (! client_data->is_more) {
        //Close the HTTP if no more data.
        ADDLOG_INFO(LOG_FEATURE_HTTP_CLIENT, "close http channel\r\n");
        httpclient_close(client);
    }
    return (ret >= 0) ? 0 : -1;
}

int utils_get_response_code(httpclient_t *client)
{
    return client->response_code;
}

iotx_err_t iotx_post(
            httpclient_t *client,
            const char *url,
            int port,
            const char *ca_crt,
            uint32_t timeout_ms,
            httpclient_data_t *client_data)
{

    return httpclient_common(client, url, port, ca_crt, HTTPCLIENT_POST, timeout_ms, client_data);
}

//void mylog12(const char *s) {
//
//    	ADDLOG_INFO(LOG_FEATURE_HTTP_CLIENT, s);
//}
static void httprequest_thread( beken_thread_arg_t arg )
{
    httprequest_t *request = (httprequest_t *)arg;

    httpclient_t *client = &request->client;
    const char *url = request->url;
    const char *header = request->header;
    int port = request->port;
    const char *ca_crt = request->ca_crt;
    //uint32_t timeout = request->timeout;
    httpclient_data_t *client_data = &request->client_data;
    int method = request->method;
    int timeout_ms = request->timeout;

    //addLog("start request thread\r\n");
    //rtos_delay_milliseconds(500);


    if (header && header[0]){
        HTTPClient_SetCustomHeader(client, header);  //Sets the custom header if needed.
    }

    //addLog("after HTTPClient_SetCustomHeader\r\n");
    //rtos_delay_milliseconds(500);

    iotx_time_t timer;
    int ret = 0;
    char host[HTTPCLIENT_MAX_HOST_LEN] = { 0 };

    request->state = 0;

    if (0 == client->net.handle) {
        //Establish connection if no.
        //addLog("before httpclient_parse_host\r\n");
        //rtos_delay_milliseconds(500);
    	ret = httpclient_parse_host(url, host, &port, sizeof(host));

        if (ret != SUCCESS_RETURN){
            request->state = -1;
            if (request->data_callback){
                request->data_callback(request);
            }
            goto exit;
        }

    	ADDLOG_INFO(LOG_FEATURE_HTTP_CLIENT, "host: '%s', port: %d", host, port);
        //rtos_delay_milliseconds(500);

    	iotx_net_init(&client->net, host, port, ca_crt);
        //addLog("after iotx_net_init\r\n");
        //rtos_delay_milliseconds(500);

    	ret = httpclient_connect(client);
        //addLog("after httpclient_connect %d\r\n", ret);
        //rtos_delay_milliseconds(500);
    	if (0 != ret) {
            ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "httpclient_connect is error,ret = %d", ret);
            httpclient_close(client);
            request->state = -1;
            if (request->data_callback){
                request->data_callback(request);
            }
            goto exit;
    	}

        ret = httpclient_send_request(client, url, method, client_data);
        if (0 != ret) {
            ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "httpclient_send_request is error,ret = %d", ret);
            httpclient_close(client);
            request->state = -1;
            if (request->data_callback){
                request->data_callback(request);
            }
            goto exit;
        }
    }
    //addLog("httpclient_connect ret = %d", ret);
    //rtos_delay_milliseconds(500);
    request->state = 0;  // start
    request->client_data.response_buf_filled = 0;
    if (request->data_callback){
        request->data_callback(request);
    }

    iotx_time_init(&timer);
    utils_time_countdown_ms(&timer, timeout_ms);

    //addLog("rx buflen %d", client_data->response_buf_len);
    //rtos_delay_milliseconds(500);

    if ((NULL != client_data->response_buf)
         || (0 != client_data->response_buf_len)) {
        do {
            // parse headers, fill client_data->response_buf up to max client_data->response_buf_len-1
            ret = httpclient_recv_response(client, iotx_time_left(&timer), client_data);
            //addLog("httpclient_recv_response is ret = %d is_more %d", ret, client_data->is_more);
            //rtos_delay_milliseconds(500);
            if (ret < 0) {
                ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "httpclient_recv_response is error,ret = %d", ret);
                httpclient_close(client);
                request->state = -2;
                if (request->data_callback){
                    request->data_callback(request);
                }
                // close & leave
                break;
            }
            request->state = 1;
            if (request->data_callback){
                //addLog("calling callback");
                //rtos_delay_milliseconds(500);
                if (request->data_callback(request)){
                    // abort on user request
                    // close & leave
                    break;
                }
            }
        } while (client_data->is_more);
    } else {
		// must read out somewhere data, otherwise lwip will fail at lwip_close and it wont free socket
		// and then it will soon run out of the sockets and break networking
		int c_read;
		int ret;
		c_read = 0;
		do {
			ret = client->net.doRead(&client->net, host, sizeof(host), iotx_time_left(&timer));
			if(ret>0)
				c_read += ret;
		} while(ret > 0);
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "httpclient - no response buff, skipped %i",c_read);
    }
exit:
    //addLog("close http channel");
    httpclient_close(client);
    request->state = 2;  // complete
    request->client_data.response_buf_filled = 0;
    if (request->data_callback){
        request->data_callback(request);
    }
	// free if required
	httpclient_freeMemory(request);
    // remove this thread
    rtos_delete_thread( NULL );
    return;
}


//////////////////////////////////////
// our async stuff
int HTTPClient_Async_SendGeneric(httprequest_t *request){
    OSStatus err = kNoErr;
    err = rtos_create_thread( NULL, BEKEN_APPLICATION_PRIORITY, 
									"httprequest", 
									(beken_thread_function_t)httprequest_thread,
									0x800,
									(beken_thread_arg_t)request );
    if(err != kNoErr)
    {
       ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "create \"httprequest\" thread failed!\r\n");
       return -1;
    }

    return 0;
}

// The malloc below is not responsible for 88 bytes mem leak in HTTP client
// It is elsewhere
//#define DBG_HTTPCLIENT_MEMLEAK 1
#if DBG_HTTPCLIENT_MEMLEAK
char tmp[125];
httprequest_t testreq;
#else

#endif
int HTTPClient_Async_SendGet(const char *url_in){
	httprequest_t *request;
	httpclient_t *client;
	httpclient_data_t *client_data;
	char *url;

	// it must be copied, but we can free it automatically later
#if DBG_HTTPCLIENT_MEMLEAK
	strcpy(tmp,url_in);
	url = tmp;
#else
	url = test_strdup(url_in);
#endif
	if(url==0) {
		ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "HTTPClient_Async_SendGet for %s, failed to alloc URL memory\r\n");
		return;
	}

#if DBG_HTTPCLIENT_MEMLEAK
	request = &testreq;
#else
	request = (httprequest_t *) malloc(sizeof(httprequest_t));
#endif
	if(url==0) {
		ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT, "HTTPClient_Async_SendGet for %s, failed to alloc request memory\r\n");
		return;
	}

    ADDLOG_INFO(LOG_FEATURE_HTTP_CLIENT, "HTTPClient_Async_SendGet for %s, sizeof(httprequest_t) == %i!\r\n",
		url_in,sizeof(httprequest_t));

	memset(request, 0, sizeof(*request));
	request->flags |= HTTPREQUEST_FLAG_FREE_SELFONDONE;
	request->flags |= HTTPREQUEST_FLAG_FREE_URLONDONE;
	client = &request->client;
	client_data = &request->client_data;

	client_data->response_buf = 0;  //Sets a buffer to store the result.
	client_data->response_buf_len = 0;  //Sets the buffer size.
	HTTPClient_SetCustomHeader(client, "");  //Sets the custom header if needed.
	client_data->post_buf = "";  //Sets the user data to be posted.
	client_data->post_buf_len = 0;  //Sets the post data length.
	client_data->post_content_type = "text/csv";  //Sets the content type.
	request->data_callback = 0; 
	request->port = 80;//HTTP_PORT;
	request->url = url;
	request->method = HTTPCLIENT_GET; 
	request->timeout = 10000;
	HTTPClient_Async_SendGeneric(request);


    return 0;
}







