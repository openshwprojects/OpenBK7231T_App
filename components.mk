ifeq ($(TARGET_PLATFORM),bk7231n)

CFG_USE_MQTT_TLS ?= 1

ifeq ($(CFG_USE_MQTT_TLS),1)

MBEDTLS_DIR = $(TOP_DIR)/apps/$(APP_BIN_NAME)/output/mbedtls-2.28.5
INCLUDES := -I$(MBEDTLS_DIR)/include -I$(TOP_DIR)/apps/$(APP_BIN_NAME)/src $(INCLUDES)
MQTT_TLS_DEFS += -DMQTT_USE_TLS=1
MQTT_TLS_DEFS += -DLWIP_ALTCP=1
MQTT_TLS_DEFS += -DLWIP_ALTCP_TLS=1
MQTT_TLS_DEFS += -DLWIP_ALTCP_TLS_MBEDTLS=1
MQTT_TLS_DEFS += -DMEMP_NUM_ALTCP_PCB=4
MQTT_TLS_DEFS += -DMBEDTLS_CONFIG_FILE='"user_mbedtls_config.h"'
CPPDEFINES += $(MQTT_TLS_DEFS) -Wno-misleading-indentation
OSFLAGS += $(MQTT_TLS_DEFS)
LFLAGS += -Wl,--print-memory-usage

SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/apps/altcp_tls/altcp_tls_mbedtls.c
SRC_C += ./beken378/func/lwip_intf/lwip-2.1.3/src/apps/altcp_tls/altcp_tls_mbedtls_mem.c
SRC_C += ${MBEDTLS_DIR}/library/ssl_tls.c
SRC_C += ${MBEDTLS_DIR}/library/x509_crt.c
SRC_C += ${MBEDTLS_DIR}/library/entropy.c
SRC_C += ${MBEDTLS_DIR}/library/chachapoly.c
SRC_C += ${MBEDTLS_DIR}/library/ctr_drbg.c
SRC_C += ${MBEDTLS_DIR}/library/ssl_msg.c
SRC_C += ${MBEDTLS_DIR}/library/debug.c
SRC_C += ${MBEDTLS_DIR}/library/md.c
SRC_C += ${MBEDTLS_DIR}/library/sha512.c
SRC_C += ${MBEDTLS_DIR}/library/platform_util.c
SRC_C += ${MBEDTLS_DIR}/library/sha256.c
SRC_C += ${MBEDTLS_DIR}/library/sha1.c
SRC_C += ${MBEDTLS_DIR}/library/ripemd160.c
SRC_C += ${MBEDTLS_DIR}/library/md5.c
SRC_C += ${MBEDTLS_DIR}/library/cipher.c
SRC_C += ${MBEDTLS_DIR}/library/gcm.c
SRC_C += ${MBEDTLS_DIR}/library/chacha20.c
SRC_C += ${MBEDTLS_DIR}/library/ccm.c
SRC_C += ${MBEDTLS_DIR}/library/constant_time.c
SRC_C += ${MBEDTLS_DIR}/library/aes.c
SRC_C += ${MBEDTLS_DIR}/library/poly1305.c
SRC_C += ${MBEDTLS_DIR}/library/pem.c
SRC_C += ${MBEDTLS_DIR}/library/des.c
SRC_C += ${MBEDTLS_DIR}/library/asn1parse.c
SRC_C += ${MBEDTLS_DIR}/library/base64.c
SRC_C += ${MBEDTLS_DIR}/library/x509.c
SRC_C += ${MBEDTLS_DIR}/library/oid.c
SRC_C += ${MBEDTLS_DIR}/library/pkparse.c
SRC_C += ${MBEDTLS_DIR}/library/ecp.c
SRC_C += ${MBEDTLS_DIR}/library/bignum.c
SRC_C += ${MBEDTLS_DIR}/library/pk.c
SRC_C += ${MBEDTLS_DIR}/library/pk_wrap.c
SRC_C += ${MBEDTLS_DIR}/library/ecdsa.c
SRC_C += ${MBEDTLS_DIR}/library/asn1write.c
SRC_C += ${MBEDTLS_DIR}/library/hmac_drbg.c
SRC_C += ${MBEDTLS_DIR}/library/rsa.c
SRC_C += ${MBEDTLS_DIR}/library/rsa_internal.c
SRC_C += ${MBEDTLS_DIR}/library/ecp_curves.c
SRC_C += ${MBEDTLS_DIR}/library/ssl_ciphersuites.c
SRC_C += ${MBEDTLS_DIR}/library/ecdh.c
SRC_C += ${MBEDTLS_DIR}/library/dhm.c
SRC_C += ${MBEDTLS_DIR}/library/ssl_srv.c
SRC_C += ${MBEDTLS_DIR}/library/cipher_wrap.c
SRC_C += ${MBEDTLS_DIR}/library/arc4.c
SRC_C += ${MBEDTLS_DIR}/library/blowfish.c
SRC_C += ${MBEDTLS_DIR}/library/camellia.c
SRC_C += ${MBEDTLS_DIR}/library/ssl_cli.c

endif   #ifeq ($(CFG_USE_MQTT_TLS),1)
endif   #ifeq ($(TARGET_PLATFORM),bk7231n)


# Wolfssl sources
# ifeq ($(CFG_ENABLE_MQTT_TLS),1)
# SRC_TLS_C += $(WOLFSSL_DIR)/src/ssl.c
# SRC_TLS_C += $(WOLFSSL_DIR)/src/internal.c
# SRC_TLS_C += $(WOLFSSL_DIR)/src/tls.c
# SRC_TLS_C += $(WOLFSSL_DIR)/src/keys.c
# SRC_TLS_C += $(WOLFSSL_DIR)/src/wolfio.c
# SRC_TLS_C += $(WOLFSSL_DIR)/src/crl.c
# SRC_TLS_C += $(WOLFSSL_DIR)/src/ocsp.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/wc_port.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/hash.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/memory.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/asn.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/sha.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/sha3.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/rsa.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/random.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/md5.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/dh.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/arc4.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/des3.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/aes.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/sha256.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/sha512.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/integer.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/hmac.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/coding.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/error.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/sp_int.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/kdf.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/dsa.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/wolfmath.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/ecc.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/tfm.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/srp.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/chacha.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/poly1305.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/curve448.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/ed448.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/fe_448.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/ge_448.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/curve25519.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/ed25519.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/fe_operations.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/fe_low_mem.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/ge_operations.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/ge_low_mem.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/wc_encrypt.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/pwdbased.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/camellia.c
# SRC_TLS_C += $(WOLFSSL_DIR)/wolfcrypt/src/logging.c
# endif