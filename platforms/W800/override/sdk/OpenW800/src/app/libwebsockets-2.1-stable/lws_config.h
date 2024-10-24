#include "wm_config.h"

#define NDEBUG

#ifndef NDEBUG
	#ifndef _DEBUG
		#define _DEBUG
	#endif
#endif
#if TLS_CONFIG_HTTP_CLIENT_SECURE
/* Define to 1 to use wolfSSL/CyaSSL as a replacement for OpenSSL.
 * LWS_OPENSSL_SUPPORT needs to be set also for this to work. */
//#define USE_WOLFSSL

/* Also define to 1 (in addition to USE_WOLFSSL) when using the
  (older) CyaSSL library */
//#define USE_OLD_CYASSL

//#define LWS_USE_MBEDTLS
#define LWS_USE_POLARSSL
#endif

//#define LWS_WITH_PLUGINS
#define LWS_WITH_NO_LOGS

/* The Libwebsocket version */
#define LWS_LIBRARY_VERSION "2.1"

#define LWS_LIBRARY_VERSION_MAJOR 2
#define LWS_LIBRARY_VERSION_MINOR 1
#define LWS_LIBRARY_VERSION_PATCH 0
/* LWS_LIBRARY_VERSION_NUMBER looks like 1005001 for e.g. version 1.5.1 */
#define LWS_LIBRARY_VERSION_NUMBER (LWS_LIBRARY_VERSION_MAJOR*1000000)+(LWS_LIBRARY_VERSION_MINOR*1000)+LWS_LIBRARY_VERSION_PATCH

/* The current git commit hash that we're building from */
//#define LWS_BUILD_HASH "${LWS_BUILD_HASH}"

#if TLS_CONFIG_HTTP_CLIENT_SECURE

/* Build with OpenSSL support */
#define LWS_OPENSSL_SUPPORT

/* The client should load and trust CA root certs it finds in the OS */
//#define LWS_SSL_CLIENT_USE_OS_CA_CERTS

/* Sets the path where the client certs should be installed. */
//#define LWS_OPENSSL_CLIENT_CERTS "${LWS_OPENSSL_CLIENT_CERTS}"
#endif
/* Turn off websocket extensions */
#define LWS_NO_EXTENSIONS

/* Enable libev io loop */
//#define LWS_USE_LIBEV

/* Enable libuv io loop */
//#define LWS_USE_LIBUV

/* Build with support for ipv6 */
//#define LWS_USE_IPV6

/* Build with support for UNIX domain socket */
//#define LWS_USE_UNIX_SOCK

/* Build with support for HTTP2 */
//#define LWS_USE_HTTP2

/* Turn on latency measuring code */
//#define LWS_LATENCY

/* Don't build the daemonizeation api */
#define LWS_NO_DAEMONIZE

/* Build without server support */
#define LWS_NO_SERVER

/* Build without client support */
//#define LWS_NO_CLIENT

/* If we should compile with MinGW support */
//#define LWS_MINGW_SUPPORT

/* Use the BSD getifaddrs that comes with libwebsocket, for uclibc support */
//#define LWS_BUILTIN_GETIFADDRS

/* use SHA1() not internal libwebsockets_SHA1 */
//#define LWS_SHA1_USE_OPENSSL_NAME

/* SSL server using ECDH certificate */
//#define LWS_SSL_SERVER_WITH_ECDH_CERT
//#define LWS_HAVE_SSL_CTX_set1_param
//#define LWS_HAVE_X509_VERIFY_PARAM_set1_host

//#define LWS_HAVE_UV_VERSION_H

/* CGI apis */
//#define LWS_WITH_CGI

/* whether the Openssl is recent enough, and / or built with, ecdh */
//#define LWS_HAVE_OPENSSL_ECDH_H

/* HTTP Proxy support */
//#define LWS_WITH_HTTP_PROXY

/* Http access log support */
//#define LWS_WITH_ACCESS_LOG
//#define LWS_WITH_SERVER_STATUS

//#define LWS_WITH_STATEFUL_URLDECODE

/* Maximum supported service threads */
#define LWS_MAX_SMP 1

/* Lightweight JSON Parser */
//#define LWS_WITH_LEJP

/* SMTP */
//#define LWS_WITH_SMTP
