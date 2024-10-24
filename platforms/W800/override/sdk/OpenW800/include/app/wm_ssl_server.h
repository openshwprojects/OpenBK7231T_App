#ifndef _SSL_SERVER_H_
#define _SSL_SERVER_H_

#include "wm_config.h"
#include "HTTPClientWrapper.h"

#if 1//for doxygen
//#if TLS_CONFIG_SERVER_SIDE_SSL

#if TLS_CONFIG_USE_POLARSSL
#include "polarssl/config.h"
#include "polarssl/ssl.h"

#error "PolaSSL does not support ssl server now!"
#elif TLS_CONFIG_USE_MBEDTLS
typedef void tls_ssl_key_t;
#endif
//key type for tls_ssl_server_init
#define KEY_RSA	1
#define	KEY_ECC	2
#define KEY_DH	3

/**
 * @defgroup APP_APIs APP APIs
 * @brief APP APIs
 */

/**
 * @addtogroup APP_APIs
 * @{
 */

/**
 * @defgroup SSL_SERVER_APIs SSL SERVER APIs
 * @brief SSL Server APIs
 */

/**
 * @addtogroup SSL_SERVER_APIs
 * @{
 */

/**
 * @brief          This function is used to initialize SSL Server
 *
 * @param[in]      *arg     proto version: 0 - sslv3
 *                                         1 - tls1.0
 *                                         2 - tls1.1
 *                                         3 - tls1.2
 *
 * @retval         0     success
 * @retval         other failed
 *
 * @note           None
 */
int tls_ssl_server_init(void * arg);

/**
 * @brief          This function is used to set SSL keys
 *
 * @param[in]      **keys     SSL key pointer
 * @param[in]      *certBuf   SSL certificate
 * @param[in]      certLen    SSL certificate length
 * @param[in]      *privBuf   SSL private key
 * @param[in]      privLen    SSL private key length
 * @param[in]      *CAbuf     CA certificate
 * @param[in]      CAlen      CA certificate length
 * @param[in]      keyType    key type: KEY_RSA,KEY_ECC,KEY_DH
 *
 * @retval         0     success
 * @retval         other failed
 *
 * @note           None
 */
int tls_ssl_server_load_keys(tls_ssl_key_t **keys, unsigned char *certBuf,
			int32 certLen, unsigned char *privBuf, int32 privLen,
			unsigned char *CAbuf, int32 CAlen, int keyType);

/**
 * @brief          This function is used to set SSL Server working
 *
 * @param[in]      **ssl_p      SSL hanlde
 * @param[in]      fd           socket number
 * @param[in]      *keys        SSL keys
 *
 * @retval         0     success
 * @retval         other failed
 *
 * @note           None
 */
int tls_ssl_server_handshake(tls_ssl_t **ssl_p, int fd, tls_ssl_key_t *keys);

/**
 * @brief          This function is used to send data
 *
 * @param[in]      *ssl         SSL hanlde
 * @param[in]      s            socket number
 * @param[in]      *sndbuf      send buffer
 * @param[in]      len          send length
 * @param[in]      flags        some flags
 *
 * @retval         > 0     success
 * @retval         <=0     failed
 *
 * @note           None
 */
int tls_ssl_server_send(tls_ssl_t *ssl, int s,char *sndbuf, int len,int flags);

/**
 * @brief          This function is used to receive data
 *
 * @param[in]      *ssl         SSL hanlde
 * @param[in]      s            socket number
 * @param[in]      *buf         receive buffer
 * @param[in]      len          receive buffer length
 * @param[in]      flags        some flags
 *
 * @retval         > 0     success
 * @retval         <=0     failed
 *
 * @note           None
 */
int tls_ssl_server_recv(tls_ssl_t *ssl,int s,char *buf, int len,int flags);

/**
 * @brief          This function is used to close connection
 *
 * @param[in]      *ssl         SSL hanlde
 * @param[in]      s            socket number
 *
 * @return         None
 *
 * @note           None
 */
void tls_ssl_server_close_conn(tls_ssl_t *ssl, int s);

/**
 * @brief          This function is used to close SSL Server
 *
 * @param[in]      *keys        SSL keys
 *
 * @retval         0         success
 * @retval         other     failed
 *
 * @note           None
 */
int tls_ssl_server_close(tls_ssl_key_t * keys);

/**
 * @}
 */

/**
 * @}
 */

#endif /*TLS_CONFIG_SERVER_SIDE_SSL*/
#endif /*_SSL_SERVER_H_*/

