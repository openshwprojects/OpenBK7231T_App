
#ifndef WM_CRYPTO_HARD_MBED_H
#define WM_CRYPTO_HARD_MBED_H

#include "mbedtls/bignum.h"

#define MAX_HARD_EXPTMOD_BITLEN   (2048)

int tls_crypto_mbedtls_exptmod( mbedtls_mpi *X, const mbedtls_mpi *A, const mbedtls_mpi *E, const mbedtls_mpi *N);


#endif

