/******************************************************************************
 * @version V1.0.0
 * @date    05-Oct-2023
 * 
 * This file contains specific configuration for mbedtls 
 * Due to environment limitations there is only one version of TSL 
 * and only one cipher enabled:
 * TSL VERSION: TLSv1.2
 * TSL CIPHER : TLS-ECDHE-RSA-WITH-AES-128-GCM-SHA256
 *
 * This is a common configuration supported by the mosquitto MQTT server
 *
 * Tested only with LWIP MQTT client application on BK7231N platform
 * It's possible that it will also work on other platforms, 
 * but I don't have specific hardware to test.
 *
 * The web server MQTT page has been updated to specify whether MQTT 
 * uses TSL and if the certificate needs to be validated.
 * The CA certificate or public certificate (in case of self-signed)
 * must be uploaded in PEM format to LFS
 *
 * To validate the certificate dates, the NTP driver must be enabled,
 * otherwise the build date will be used to validate.
 *
 * You can use Mqtt TSL without a CA or public certificate if you disable 
 * validation, but this is not recommended.
 * Your client will be vulnerable to the MIT attack.
 *
 * Additionally, an option to disable the web app has been added to 
 * strengthen security. After connected to mqtt use 
 * cmnd/<topic>/WebServer 0 to disable web interface
 * cmnd/<topic>/WebServer 1 to enable web interface
 * Communication only with secure mqtt connection
 *
 * Author: alexsandroz@gmail.com
 * 
 ******************************************************************************/


#ifndef USER_MBEDTLS_CONFIG_H
#define USER_MBEDTLS_CONFIG_H

#if MQTT_USE_TLS

#include "mbedtls/config.h"

// Plataform specific
#undef  MBEDTLS_FS_IO
#undef  MBEDTLS_NET_C
#define MBEDTLS_TIMING_C
#define MBEDTLS_HAVE_TIME_DATE
#define MBEDTLS_PLATFORM_GMTIME_R_ALT
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ENTROPY_HARDWARE_ALT
#define MBEDTLS_MPI_WINDOW_SIZE            1
#define MBEDTLS_MPI_MAX_SIZE             512
#define MBEDTLS_ECP_WINDOW_SIZE            2
#define MBEDTLS_SSL_MAX_CONTENT_LEN     4096

// Modes
#define MBEDTLS_SSL_CLI_C  // Only client enabled
#undef  MBEDTLS_SSL_SRV_C 

// Protos
#undef  MBEDTLS_SSL_PROTO_SSL3
#undef  MBEDTLS_SSL_PROTO_TLS1
#undef  MBEDTLS_SSL_PROTO_TLS1_1
#define MBEDTLS_SSL_PROTO_TLS1_2  // Only TLS1.2 enabled
#undef  MBEDTLS_SSL_PROTO_TLS1_3_EXPERIMENTAL
#undef  MBEDTLS_SSL_PROTO_DTLS
#undef  MBEDTLS_SSL_DTLS_ANTI_REPLAY

// Enabled Ciphers
#define MBEDTLS_RSA_C
#define MBEDTLS_SHA256_C
#define MBEDTLS_SHA256_SMALLER
#define MBEDTLS_AES_C
#define MBEDTLS_AES_ROM_TABLES
#define MBEDTLS_AES_FEWER_TABLES
#define MBEDTLS_ENTROPY_FORCE_SHA256

//Disabled ciphers
#undef  MBEDTLS_ARC4_C
#undef  MBEDTLS_BLOWFISH_C
#undef  MBEDTLS_CAMELLIA_C
#undef  MBEDTLS_ARIA_C
#undef  MBEDTLS_DES_C
#undef  MBEDTLS_CCM_C
#undef  MBEDTLS_MD2_C
#undef  MBEDTLS_MD4_C
#undef  MBEDTLS_MD5_C
#undef  MBEDTLS_RIPEMD160_C
#undef  MBEDTLS_SHA1_C
#undef  MBEDTLS_SHA512_C
#define MBEDTLS_SHA512_NO_SHA384
#undef  MBEDTLS_CHACHA20_C
#undef  MBEDTLS_CHACHAPOLY_C
#undef  MBEDTLS_POLY1305_C
#undef  MBEDTLS_CIPHER_NULL_CIPHER
#undef  MBEDTLS_ENABLE_WEAK_CIPHERSUITES
#define MBEDTLS_REMOVE_3DES_CIPHERSUITES
#undef  MBEDTLS_TLS_DEFAULT_ALLOW_SHA1_IN_KEY_EXCHANGE

// Curves //
#undef  MBEDTLS_ECP_DP_SECP192R1_ENABLED
#undef  MBEDTLS_ECP_DP_SECP224R1_ENABLED
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED  //Only SECP256R1
#undef  MBEDTLS_ECP_DP_SECP384R1_ENABLED
#undef  MBEDTLS_ECP_DP_SECP521R1_ENABLED
#undef  MBEDTLS_ECP_DP_SECP192K1_ENABLED
#undef  MBEDTLS_ECP_DP_SECP224K1_ENABLED
#undef  MBEDTLS_ECP_DP_SECP256K1_ENABLED
#undef  MBEDTLS_ECP_DP_BP256R1_ENABLED
#undef  MBEDTLS_ECP_DP_BP384R1_ENABLED
#undef  MBEDTLS_ECP_DP_BP512R1_ENABLED
#undef  MBEDTLS_ECP_DP_CURVE25519_ENABLED
#undef  MBEDTLS_ECP_DP_CURVE448_ENABLED

// Block mode
#define MBEDTLS_GCM_C              //Only GCM
#undef  MBEDTLS_CIPHER_MODE_CBC
#undef  MBEDTLS_CIPHER_MODE_CFB
#undef  MBEDTLS_CIPHER_MODE_CTR
#undef  MBEDTLS_CIPHER_MODE_OFB
#undef  MBEDTLS_CIPHER_MODE_XTS

// Exchange Key //
#define MBEDTLS_DHM_C
#define MBEDTLS_ECDH_C
#undef  MBEDTLS_ECDSA_C
#undef  MBEDTLS_ECJPAKE_C
#undef  MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
#undef  MBEDTLS_KEY_EXCHANGE_DHE_PSK_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED
#undef  MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED
#undef  MBEDTLS_KEY_EXCHANGE_RSA_ENABLED
#undef  MBEDTLS_KEY_EXCHANGE_DHE_RSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED
#undef  MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
#undef  MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA_ENABLED
#undef  MBEDTLS_KEY_EXCHANGE_ECDH_RSA_ENABLED
#undef  MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED

// Define only on development //
#if 0
#define MBEDTLS_DEBUG_C
#define MBEDTLS_SELF_TEST
#define MBEDTLS_SSL_ALL_ALERT_MESSAGES
#define MBEDTLS_SSL_RECORD_CHECKING
#define MBEDTLS_SSL_CONTEXT_SERIALIZATION
#define MBEDTLS_SSL_DEBUG_ALL
#define MBEDTLS_VERSION_FEATURES
#define MBEDTLS_CERTS_C
#define MBEDTLS_MEMORY_BUFFER_ALLOC_C
#define MBEDTLS_MEMORY_BACKTRACE
#else
#undef  MBEDTLS_DEBUG_C
#undef  MBEDTLS_SELF_TEST
#undef  MBEDTLS_SSL_ALL_ALERT_MESSAGES
#undef  MBEDTLS_SSL_RECORD_CHECKING
#undef  MBEDTLS_SSL_CONTEXT_SERIALIZATION
#undef  MBEDTLS_SSL_DEBUG_ALL
#undef  MBEDTLS_VERSION_FEATURES
#undef  MBEDTLS_CERTS_C
#undef  MBEDTLS_MEMORY_BUFFER_ALLOC_C
#undef  MBEDTLS_MEMORY_BACKTRACE
#endif

//Disabled functions
#undef  MBEDTLS_SSL_KEEP_PEER_CERTIFICATE
#undef  MBEDTLS_PKCS1_V21
#undef  MBEDTLS_GENPRIME
#undef  MBEDTLS_X509_RSASSA_PSS_SUPPORT

#endif   //MQTT_USE_TLS
#endif   //USER_MBEDTLS_CONFIG_H