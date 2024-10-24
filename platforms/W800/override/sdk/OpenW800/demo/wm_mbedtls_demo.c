/*****************************************************************************
*
* File Name : wm_mbedtls_demo.c
*
* Description: ssl client demo function
*
* Copyright (c) 2015 Winner Micro Electronic Design Co., Ltd.
* All rights reserved.
*
* Author : LiLimin
*
* Date : 2015-3-24
*****************************************************************************/
#include <string.h>
#include "wm_include.h"
#include "wm_demo.h"
#include "lwip/netif.h"
#include "wm_netif.h"
#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

#if DEMO_MBEDTLS

#define MBEDTLS_DEMO_TASK_PRIO             38
#define MBEDTLS_DEMO_TASK_SIZE             2048
#define MBEDTLS_DEMO_QUEUE_SIZE            4

#define MBEDTLS_DEMO_RECV_BUF_LEN          1024

#define MBEDTLS_DEMO_CMD_START             0x1

#define MBEDTLS_DEMO_SERVER               "www.tencent.com"
#define MBEDTLS_DEMO_PORT                 "443"

#define MBEDTLS_DEMO_USE_CERT                    0

static bool mbedtls_demo_inited = FALSE;
static OS_STK mbedtls_demo_task_stk[MBEDTLS_DEMO_TASK_SIZE];
static tls_os_queue_t *mbedtls_demo_task_queue = NULL;

static const char *http_request = "GET /legal/html/zh-cn/index.html HTTP/1.0\r\n"
                                   "Host: "MBEDTLS_DEMO_SERVER"\r\n"
                                   "User-Agent: ssl_client\r\n"
                                   "\r\n";

#if MBEDTLS_DEMO_USE_CERT
static const char mbedtls_demos_pem[] =                                 \
"-----BEGIN CERTIFICATE-----\r\n"                                       \
"MIIDhzCCAm+gAwIBAgIBADANBgkqhkiG9w0BAQUFADA7MQswCQYDVQQGEwJOTDER\r\n"  \
"MA8GA1UEChMIUG9sYXJTU0wxGTAXBgNVBAMTEFBvbGFyU1NMIFRlc3QgQ0EwHhcN\r\n"  \
"MTEwMjEyMTQ0NDAwWhcNMjEwMjEyMTQ0NDAwWjA7MQswCQYDVQQGEwJOTDERMA8G\r\n"  \
"A1UEChMIUG9sYXJTU0wxGTAXBgNVBAMTEFBvbGFyU1NMIFRlc3QgQ0EwggEiMA0G\r\n"  \
"CSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDA3zf8F7vglp0/ht6WMn1EpRagzSHx\r\n"  \
"mdTs6st8GFgIlKXsm8WL3xoemTiZhx57wI053zhdcHgH057Zk+i5clHFzqMwUqny\r\n"  \
"50BwFMtEonILwuVA+T7lpg6z+exKY8C4KQB0nFc7qKUEkHHxvYPZP9al4jwqj+8n\r\n"  \
"YMPGn8u67GB9t+aEMr5P+1gmIgNb1LTV+/Xjli5wwOQuvfwu7uJBVcA0Ln0kcmnL\r\n"  \
"R7EUQIN9Z/SG9jGr8XmksrUuEvmEF/Bibyc+E1ixVA0hmnM3oTDPb5Lc9un8rNsu\r\n"  \
"KNF+AksjoBXyOGVkCeoMbo4bF6BxyLObyavpw/LPh5aPgAIynplYb6LVAgMBAAGj\r\n"  \
"gZUwgZIwDAYDVR0TBAUwAwEB/zAdBgNVHQ4EFgQUtFrkpbPe0lL2udWmlQ/rPrzH\r\n"  \
"/f8wYwYDVR0jBFwwWoAUtFrkpbPe0lL2udWmlQ/rPrzH/f+hP6Q9MDsxCzAJBgNV\r\n"  \
"BAYTAk5MMREwDwYDVQQKEwhQb2xhclNTTDEZMBcGA1UEAxMQUG9sYXJTU0wgVGVz\r\n"  \
"dCBDQYIBADANBgkqhkiG9w0BAQUFAAOCAQEAuP1U2ABUkIslsCfdlc2i94QHHYeJ\r\n"  \
"SsR4EdgHtdciUI5I62J6Mom+Y0dT/7a+8S6MVMCZP6C5NyNyXw1GWY/YR82XTJ8H\r\n"  \
"DBJiCTok5DbZ6SzaONBzdWHXwWwmi5vg1dxn7YxrM9d0IjxM27WNKs4sDQhZBQkF\r\n"  \
"pjmfs2cb4oPl4Y9T9meTx/lvdkRYEug61Jfn6cA+qHpyPYdTH+UshITnmp5/Ztkf\r\n"  \
"m/UTSLBNFNHesiTZeH31NcxYGdHSme9Nc/gfidRa0FLOCfWxRlFqAI47zG9jAQCZ\r\n"  \
"7Z2mCGDNMhjQc+BYcdnl0lPXjdDK6V0qCg1dVewhUBcW5gZKzV7e9+DpVA==\r\n"      \
"-----END CERTIFICATE-----\r\n";
#endif

extern struct netif *tls_get_netif(void);
extern int wm_printf(const char *fmt, ...);

static void mbedtls_demo_net_status(u8 status)
{
    struct netif *netif = tls_get_netif();

    switch(status)
    {
    case NETIF_WIFI_JOIN_FAILED:
        wm_printf("sta join net failed\n");
        break;
    case NETIF_WIFI_DISCONNECTED:
        wm_printf("sta net disconnected\n");
        break;
    case NETIF_IP_NET_UP:
        wm_printf("sta ip: %v\n", netif->ip_addr.addr);
        tls_os_queue_send(mbedtls_demo_task_queue, (void *)MBEDTLS_DEMO_CMD_START, 0);
        break;
    default:
        break;
    }
}

#if defined(MBEDTLS_DEBUG_C)
#define DEBUG_LEVEL 3

static void ssl_client_debug( void *ctx, int level,
                              const char *file, int line,
                              const char *str )
{
    ((void) level);

    mbedtls_fprintf( (FILE *) ctx, "%s:%04d: %s", file, line, str );
    fflush(  (FILE *) ctx  );
}
#endif

static int ssl_client_demo(void)
{
    int ret = 1, len;
    int exit_code = MBEDTLS_EXIT_FAILURE;
    mbedtls_net_context server_fd;
    unsigned char buf[MBEDTLS_DEMO_RECV_BUF_LEN];
    const char *pers = "ssl_client";

    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
#if MBEDTLS_DEMO_USE_CERT
    mbedtls_x509_crt cacert;
#endif

#if defined(MBEDTLS_DEBUG_C)
    mbedtls_debug_set_threshold( DEBUG_LEVEL );
#endif

    /*
     * 0. Initialize the RNG and the session data
     */
    mbedtls_net_init( &server_fd );
    mbedtls_ssl_init( &ssl );
    mbedtls_ssl_config_init( &conf );
#if MBEDTLS_DEMO_USE_CERT
    mbedtls_x509_crt_init( &cacert );
#endif
    mbedtls_ctr_drbg_init( &ctr_drbg );

    mbedtls_printf( "\n  . Seeding the random number generator..." );
    fflush( stdout );

    mbedtls_entropy_init( &entropy );
    if( ( ret = mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret );
        goto exit;
    }

    mbedtls_printf( " ok\n" );

#if MBEDTLS_DEMO_USE_CERT
    /*
     * 0. Initialize certificates
     */
    mbedtls_printf( "  . Loading the CA root certificate ..." );
    fflush( stdout );

    ret = mbedtls_x509_crt_parse( &cacert, (const unsigned char *) mbedtls_demos_pem,
                          sizeof(mbedtls_demos_pem) );
    if( ret < 0 )
    {
        mbedtls_printf( " failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret );
        goto exit;
    }

    mbedtls_printf( " ok (%d skipped)\n", ret );
#endif

    /*
     * 1. Start the connection
     */
    mbedtls_printf( "  . Connecting to tcp/%s/%s...", MBEDTLS_DEMO_SERVER, MBEDTLS_DEMO_PORT );
    fflush( stdout );

    if( ( ret = mbedtls_net_connect( &server_fd, MBEDTLS_DEMO_SERVER,
                                         MBEDTLS_DEMO_PORT, MBEDTLS_NET_PROTO_TCP ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_net_connect returned %d\n\n", ret );
        goto exit;
    }

    mbedtls_printf( " ok\n" );

    /*
     * 2. Setup stuff
     */
    mbedtls_printf( "  . Setting up the SSL/TLS structure..." );
    fflush( stdout );

    if( ( ret = mbedtls_ssl_config_defaults( &conf,
                    MBEDTLS_SSL_IS_CLIENT,
                    MBEDTLS_SSL_TRANSPORT_STREAM,
                    MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret );
        goto exit;
    }

    mbedtls_printf( " ok\n" );

    /* OPTIONAL is not optimal for security,
     * but makes interop easier in this simplified example */
    mbedtls_ssl_conf_authmode( &conf, MBEDTLS_SSL_VERIFY_NONE );
#if MBEDTLS_DEMO_USE_CERT
    mbedtls_ssl_conf_ca_chain( &conf, &cacert, NULL );
#endif
    mbedtls_ssl_conf_rng( &conf, mbedtls_ctr_drbg_random, &ctr_drbg );

#if defined(MBEDTLS_DEBUG_C)
    mbedtls_ssl_conf_dbg( &conf, ssl_client_debug, stdout );
#endif

    if( ( ret = mbedtls_ssl_setup( &ssl, &conf ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ssl_setup returned %d\n\n", ret );
        goto exit;
    }

    if( ( ret = mbedtls_ssl_set_hostname( &ssl, MBEDTLS_DEMO_SERVER ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", ret );
        goto exit;
    }

    mbedtls_ssl_set_bio( &ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL );

    /*
     * 4. Handshake
     */
    mbedtls_printf( "  . Performing the SSL/TLS handshake..." );
    fflush( stdout );

    while( ( ret = mbedtls_ssl_handshake( &ssl ) ) != 0 )
    {
        if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
            mbedtls_printf( " failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", -ret );
            goto exit;
        }
    }

    mbedtls_printf( " ok\n" );

    /*
     * 3. Write the GET request
     */
    mbedtls_printf( "  > Write to server:" );
    fflush( stdout );

    len = sprintf( (char *) buf, http_request );

    while( ( ret = mbedtls_ssl_write( &ssl, buf, len ) ) <= 0 )
    {
        if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
            mbedtls_printf( " failed\n  ! mbedtls_ssl_write returned %d\n\n", ret );
            goto exit;
        }
    }

    len = ret;
    mbedtls_printf( "\r\n\r\n %d bytes written\n\n%s", len, (char *) buf );

    /*
     * 7. Read the HTTP response
     */
    mbedtls_printf( "  < Read from server:" );
    fflush( stdout );

    do
    {
        len = sizeof( buf ) - 1;
        memset( buf, 0, sizeof( buf ) );
        ret = mbedtls_ssl_read( &ssl, buf, len );

        if( ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE )
            continue;

        if( ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY )
            break;

        if( ret < 0 )
        {
            //mbedtls_printf( "failed\n  ! mbedtls_ssl_read returned %d\n\n", ret );
            break;
        }

        if( ret == 0 )
        {
            mbedtls_printf( "\n\nEOF\n\n" );
            break;
        }

        len = ret;
        mbedtls_printf( " %d bytes read\n\n%s", len, (char *) buf );
    }
    while( 1 );

    mbedtls_ssl_close_notify( &ssl );

    exit_code = MBEDTLS_EXIT_SUCCESS;

exit:

#ifdef MBEDTLS_ERROR_C
    if( exit_code != MBEDTLS_EXIT_SUCCESS )
    {
        char error_buf[100];
        mbedtls_strerror( ret, error_buf, 100 );
        mbedtls_printf("Last error was: %d - %s\n\n", ret, error_buf );
    }
#endif

    mbedtls_net_free( &server_fd );
#if MBEDTLS_DEMO_USE_CERT
    mbedtls_x509_crt_free( &cacert );
#endif
    mbedtls_ssl_free( &ssl );
    mbedtls_ssl_config_free( &conf );
    mbedtls_ctr_drbg_free( &ctr_drbg );
    mbedtls_entropy_free( &entropy );

    return( exit_code );
}

static void mbedtls_demo_task(void *p)
{
    int ret;
    void *msg;
    struct tls_ethif *ether_if = tls_netif_get_ethif();

    if (ether_if->status)
    {
        wm_printf("sta ip: %v\n", ether_if->ip_addr.addr);
        tls_os_queue_send(mbedtls_demo_task_queue, (void *)MBEDTLS_DEMO_CMD_START, 0);
    }

    for( ; ; )
    {
        ret = tls_os_queue_receive(mbedtls_demo_task_queue, (void **)&msg, 0, 0);
        if (!ret)
        {
            switch ((u32)msg)
            {
                case MBEDTLS_DEMO_CMD_START:
                    ssl_client_demo();
                    break;
                default:
                    break;
            }
        }
    }
}


//https request demo
//This example should make STA connected to AP firstly, then access web page https://www.tencent.com/legal/html/zh-cn/index.html
int mbedtls_demo(void)
{
    if (!mbedtls_demo_inited)
    {
        tls_os_task_create(NULL, NULL, mbedtls_demo_task,
                           NULL, (void *)mbedtls_demo_task_stk, /* task's stack start address */
                           MBEDTLS_DEMO_TASK_SIZE * sizeof(u32),/* task's stack size, unit:byte */
                           MBEDTLS_DEMO_TASK_PRIO, 0);

        tls_os_queue_create(&mbedtls_demo_task_queue, MBEDTLS_DEMO_QUEUE_SIZE);

        tls_netif_add_status_event(mbedtls_demo_net_status);

        mbedtls_demo_inited = TRUE;
    }

    return WM_SUCCESS;
}

#endif

