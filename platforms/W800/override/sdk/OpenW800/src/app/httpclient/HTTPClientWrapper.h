
#ifndef HTTP_CLIENT_WRAPPER
#define HTTP_CLIENT_WRAPPER

#include "wm_config.h"
#include "lwip/arch.h"

#if TLS_CONFIG_HTTP_CLIENT_SECURE
#if TLS_CONFIG_USE_POLARSSL
#include "polarssl/config.h"
#include "polarssl/ssl.h"
#elif TLS_CONFIG_USE_MBEDTLS

#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

#endif
#endif
// Compilation mode
#define _HTTP_BUILD_WIN32            // Set Windows Build flag


///////////////////////////////////////////////////////////////////////////////
//
// Section      : Microsoft Windows Support
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

#ifdef _HTTP_BUILD_WIN32

//#pragma warning (disable: 4996) // 'function': was declared deprecated (VS 2005)
#include <stdlib.h>
#include <string.h>
//#include <memory.h>
#include <stdio.h>
#include <ctype.h>
//#include <time.h>
//#include <winsock.h>

#include "wm_type_def.h"
#include "wm_sockets.h"
#include "wm_http_client.h"
// Generic types
// Sockets (Winsock wrapper)

#define                              HTTP_ECONNRESET     (ECONNRESET) 
#define                              HTTP_EINPROGRESS    (EINPROGRESS)
#define                              HTTP_EWOULDBLOCK    (EWOULDBLOCK)

#define SOCKET_ERROR            (-1)
#define SOCKET_SSL_MORE_DATA            (-2)
#endif


///////////////////////////////////////////////////////////////////////////////
//
// Section      : Functions that are not supported by the AMT stdc framework
//                So they had to be specificaly added.
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus 
extern "C" { 
#endif

#if TLS_CONFIG_HTTP_CLIENT_SECURE
#if TLS_CONFIG_USE_POLARSSL
typedef ssl_context   tls_ssl_t;
#elif TLS_CONFIG_USE_MBEDTLS
#define MBEDTLS_DEMO_USE_CERT                    0
typedef struct _ssl_t
{
	mbedtls_ssl_context ssl;
	mbedtls_net_context server_fd;
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_ssl_config conf;
#if MBEDTLS_DEMO_USE_CERT
	mbedtls_x509_crt cacert;
#endif
}tls_ssl_t;
#endif
#endif


    // STDC Wrapper implimentation
    int                                 HTTPWrapperIsAscii              (int c);
    int                                 HTTPWrapperToUpper              (int c);
    int                                 HTTPWrapperToLower              (int c);
    int                                 HTTPWrapperIsAlpha              (int c);
    int                                 HTTPWrapperIsAlNum              (int c);
    char*                               HTTPWrapperItoa                 (char *buff,int i);
    void                                HTTPWrapperInitRandomeNumber    (void);
    long                                HTTPWrapperGetUpTime            (void);
    int                                 HTTPWrapperGetRandomeNumber     (void);
    int                                 HTTPWrapperGetSocketError       (int s);
    unsigned long                       HTTPWrapperGetHostByName        (char *name,unsigned long *address);
    int                                 HTTPWrapperShutDown             (int s,int in);  
    // SSL Wrapper prototypes
#if TLS_CONFIG_HTTP_CLIENT_SECURE
    int                                 HTTPWrapperSSLConnect           (tls_ssl_t **ssl_p,int s,const struct sockaddr *name,int namelen,char *hostname);
    int                                 HTTPWrapperSSLNegotiate         (tls_http_session_handle_t pSession,int s,const struct sockaddr *name,int namelen,char *hostname);
    int                                 HTTPWrapperSSLSend              (tls_ssl_t *ssl,int s,char *buf, int len,int flags);
    int                                 HTTPWrapperSSLRecv              (tls_ssl_t *ssl,int s,char *buf, int len,int flags);
    int                                 HTTPWrapperSSLClose             (tls_ssl_t *ssl, int s);
    int                                 HTTPWrapperSSLRecvPending       (tls_ssl_t *ssl);
#endif
    // Global wrapper Functions
#define                             IToA                            HTTPWrapperItoa
#define                             GetUpTime                       HTTPWrapperGetUpTime
#define                             SocketGetErr                    HTTPWrapperGetSocketError 
#define                             HostByName                      HTTPWrapperGetHostByName
#define                             InitRandomeNumber               HTTPWrapperInitRandomeNumber
#define                             GetRandomeNumber                HTTPWrapperGetRandomeNumber

#ifdef __cplusplus 
}
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Section      : Global type definitions
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

//#ifndef NULL
//#define NULL                         0
//#endif
//#define TRUE                         1
//#define FALSE                        0

#ifdef u_long
#undef u_long
#endif
typedef unsigned long u_long;

// Global socket structures and definitions
#define                              HTTP_INVALID_SOCKET (-1)
typedef struct sockaddr_in           HTTP_SOCKADDR_IN;
typedef struct timeval               HTTP_TIMEVAL; 
typedef struct hostent               HTTP_HOSTNET;
typedef struct sockaddr              HTTP_SOCKADDR;
typedef struct in_addr               HTTP_INADDR;


#endif // HTTP_CLIENT_WRAPPER
