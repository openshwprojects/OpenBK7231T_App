#include "wm_config.h"

#if TLS_CONFIG_SERVER_SIDE_SSL

#include "HTTPClientWrapper.h"
#include "wm_ssl_server.h"
#include "lwip/arch.h"
#include "wm_sockets.h"


#if TLS_CONFIG_USE_POLARSSL

#elif TLS_CONFIG_USE_MBEDTLS
#include "mbedtls/platform.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/certs.h"
#include "mbedtls/x509.h"
#include "mbedtls/ssl.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"

typedef struct {
    mbedtls_x509_crt srvcert;
    mbedtls_pk_context pkey;
} __tls_ssl_key_t;

static int  g_proto;

#if defined(MBEDTLS_DEBUG_C)
#define DEBUG_LEVEL 3

static void ssl_server_debug( void *ctx, int level,
                              const char *file, int line,
                              const char *str )
{
    ((void) level);

    mbedtls_printf( "%s", str );
}
#endif

int tls_ssl_server_init(void * arg)
{
	g_proto = (int)arg;

    return 0;
}


int tls_ssl_server_load_keys(tls_ssl_key_t **keys, unsigned char *certBuf,
			int32 certLen, unsigned char *privBuf, int32 privLen,
			unsigned char *CAbuf, int32 CAlen, int keyType)
{
	int ret;
	__tls_ssl_key_t *ssl_server_key = NULL;

    ssl_server_key = tls_mem_alloc(sizeof(__tls_ssl_key_t));
    if (!ssl_server_key)
        return -1;

    memset(ssl_server_key, 0, sizeof(__tls_ssl_key_t));

    mbedtls_x509_crt_init( &ssl_server_key->srvcert );
    mbedtls_pk_init( &ssl_server_key->pkey );

    //mbedtls_printf( "\n  . Loading the server cert. and key..." );
    fflush( stdout );

    ret = mbedtls_x509_crt_parse( &ssl_server_key->srvcert, (const unsigned char *) certBuf, certLen );
    if( ret != 0 )
    {
        mbedtls_printf( " failed\n  !  mbedtls_x509_crt_parse returned %x\n\n", ret );
        goto exit;
    }

    ret = mbedtls_x509_crt_parse( &ssl_server_key->srvcert, (const unsigned char *) CAbuf, CAlen );
    if( ret != 0 )
    {
        mbedtls_printf( " failed\n  !  mbedtls_x509_crt_parse returned %x\n\n", ret );
        goto exit;
    }

    ret =  mbedtls_pk_parse_key( &ssl_server_key->pkey, (const unsigned char *) privBuf, privLen, NULL, 0 );
    if( ret != 0 )
    {
        mbedtls_printf( " failed\n  !  mbedtls_pk_parse_key returned %x\n\n", ret );
        goto exit;
    }

    //mbedtls_printf( " ok\n" );

    if (keys) *keys = ssl_server_key;

    return 0;

exit:
#ifdef MBEDTLS_ERROR_C
    if( ret != 0 )
    {
        char error_buf[100];
        mbedtls_strerror( ret, error_buf, 100 );
        mbedtls_printf("Last error was: %d - %s\n\n", ret, error_buf );
    }
#endif

    mbedtls_x509_crt_free( &ssl_server_key->srvcert );
    mbedtls_pk_free( &ssl_server_key->pkey );

    tls_mem_free(ssl_server_key);
    ssl_server_key = NULL;

    return -2;
}

int tls_ssl_server_handshake(tls_ssl_t **ssl_p, int fd, tls_ssl_key_t *keys)
{
    int ret;
	const char *pers = "wm_ssls";
	tls_ssl_t *ssl_server_ctx = NULL;

    ssl_server_ctx = tls_mem_alloc(sizeof(tls_ssl_t));
    if (!ssl_server_ctx)
        return -1;

    memset(ssl_server_ctx, 0, sizeof(tls_ssl_t));

    mbedtls_ssl_init( &ssl_server_ctx->ssl );
    mbedtls_ssl_config_init( &ssl_server_ctx->conf );
    mbedtls_entropy_init( &ssl_server_ctx->entropy );
    mbedtls_ctr_drbg_init( &ssl_server_ctx->ctr_drbg );
    mbedtls_net_init( &ssl_server_ctx->server_fd );

#if defined(MBEDTLS_DEBUG_C)
    mbedtls_debug_set_threshold( DEBUG_LEVEL );
#endif

    //mbedtls_printf( "  . Seeding the random number generator..." );
    fflush( stdout );

    if( ( ret = mbedtls_ctr_drbg_seed( &ssl_server_ctx->ctr_drbg, mbedtls_entropy_func, &ssl_server_ctx->entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ctr_drbg_seed returned %x\n", ret );
        goto exit;
    }

    //mbedtls_printf( " ok\n" );

    //mbedtls_printf( "  . Setting up the SSL data...." );
    fflush( stdout );

    if( ( ret = mbedtls_ssl_config_defaults( &ssl_server_ctx->conf,
                    MBEDTLS_SSL_IS_SERVER,
                    MBEDTLS_SSL_TRANSPORT_STREAM,
                    MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ssl_config_defaults returned %x\n\n", ret );
        goto exit;
    }

    mbedtls_ssl_conf_rng( &ssl_server_ctx->conf, mbedtls_ctr_drbg_random, &ssl_server_ctx->ctr_drbg );

#if defined(MBEDTLS_DEBUG_C)
    mbedtls_ssl_conf_dbg( &ssl_server_ctx->conf, ssl_server_debug, stdout );
#endif

    mbedtls_ssl_conf_ca_chain( &ssl_server_ctx->conf, ((__tls_ssl_key_t *)keys)->srvcert.next, NULL );
    if( ( ret = mbedtls_ssl_conf_own_cert( &ssl_server_ctx->conf, &((__tls_ssl_key_t *)keys)->srvcert, &((__tls_ssl_key_t *)keys)->pkey ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ssl_conf_own_cert returned %x\n\n", ret );
        goto exit;
    }

    //mbedtls_ssl_conf_min_version( &ssl_server_ctx->conf, g_proto, g_proto );
    //mbedtls_ssl_conf_max_version( &ssl_server_ctx->conf, g_proto, g_proto );

    if( ( ret = mbedtls_ssl_setup( &ssl_server_ctx->ssl, &ssl_server_ctx->conf ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ssl_setup returned %x\n\n", ret );
        goto exit;
    }

    //mbedtls_printf( " ok\n" );

    mbedtls_net_free( &ssl_server_ctx->server_fd );
	mbedtls_ssl_session_reset( &ssl_server_ctx->ssl );

    ssl_server_ctx->server_fd.fd = fd;
    mbedtls_ssl_set_bio( &ssl_server_ctx->ssl, &ssl_server_ctx->server_fd, mbedtls_net_send, mbedtls_net_recv, NULL );

	//mbedtls_printf( "  . Performing the SSL/TLS handshake..." );
    fflush( stdout );

    while( ( ret = mbedtls_ssl_handshake( &ssl_server_ctx->ssl ) ) != 0 )
    {
        if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
            mbedtls_printf( " failed\n  ! mbedtls_ssl_handshake returned %x\n\n", ret );
#ifdef MBEDTLS_ERROR_C
            char error_buf[100];
            mbedtls_strerror( ret, error_buf, 100 );
            mbedtls_printf("Last error was: %d - %s\n\n", ret, error_buf );
#endif
            return ret;
        }
    }

    //mbedtls_printf( " ok\n" );

    if (ssl_p) *ssl_p = ssl_server_ctx;

    return 0;

exit:
#ifdef MBEDTLS_ERROR_C
    if( ret != 0 )
    {
        char error_buf[100];
        mbedtls_strerror( ret, error_buf, 100 );
        mbedtls_printf("Last error was: %d - %s\n\n", ret, error_buf );
    }
#endif

    mbedtls_ssl_free( &ssl_server_ctx->ssl );
    mbedtls_ssl_config_free( &ssl_server_ctx->conf );
    mbedtls_ctr_drbg_free( &ssl_server_ctx->ctr_drbg );
    mbedtls_entropy_free( &ssl_server_ctx->entropy );

    tls_mem_free(ssl_server_ctx);

    return -2;
}

int tls_ssl_server_send(tls_ssl_t *ssl, int s,char *sndbuf, int len,int flags)
{
    int ret;

    while( ( ret = mbedtls_ssl_write( &ssl->ssl, (const unsigned char *)sndbuf, len ) ) <= 0 )
    {
        if( ret == MBEDTLS_ERR_NET_CONN_RESET )
        {
            mbedtls_printf( " failed\n  ! peer closed the connection\n\n" );
            break;
        }

        if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
            mbedtls_printf( " failed\n  ! mbedtls_ssl_write returned %d\n\n", ret );
            break;
        }
    }

    return ret;
}

int tls_ssl_server_recv(tls_ssl_t *ssl,int s,char *buf, int len,int flags)
{
    int ret;

    do
    {
        ret = mbedtls_ssl_read( &ssl->ssl, (unsigned char *)buf, len );

        if( ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE )
            continue;

        if( ret <= 0 )
        {
            switch( ret )
            {
                case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                    mbedtls_printf( " connection was closed gracefully\n" );
                    break;

                case MBEDTLS_ERR_NET_CONN_RESET:
                    mbedtls_printf( " connection was reset by peer\n" );
                    break;

                default:
                    mbedtls_printf( " mbedtls_ssl_read returned -0x%x\n", -ret );
                    break;
            }

            break;
        }

        if( ret > 0 )
            break;
    } while( 1 );

    return ret;
}

void tls_ssl_server_close_conn(tls_ssl_t *ssl, int s)
{
    int ret;

	//mbedtls_printf( "  . Closing the connection..." );

    while( ( ret = mbedtls_ssl_close_notify( &ssl->ssl ) ) < 0 )
    {
        if( ret != MBEDTLS_ERR_SSL_WANT_READ &&
            ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
            mbedtls_printf( " failed\n  ! mbedtls_ssl_close_notify returned %x\n\n", ret );
            return;
        }
    }

    //mbedtls_printf( " ok\n" );

    mbedtls_net_free( &ssl->server_fd );
	mbedtls_ssl_free( &ssl->ssl );
    mbedtls_ssl_config_free( &ssl->conf );
    mbedtls_ctr_drbg_free( &ssl->ctr_drbg );
    mbedtls_entropy_free( &ssl->entropy );

    tls_mem_free(ssl);
}

int tls_ssl_server_close(tls_ssl_key_t * keys)
{
    if (keys)
    {
        mbedtls_pk_free( &((__tls_ssl_key_t *)keys)->pkey );
        mbedtls_x509_crt_free( &((__tls_ssl_key_t *)keys)->srvcert );

        tls_mem_free(keys);
    }

    return 0;
}

#endif

#endif

