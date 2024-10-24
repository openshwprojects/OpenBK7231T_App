/*
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif
#include "mbedtls/platform.h"

#include "wm_osal.h"
#include "wm_crypto_hard.h"

#if defined(MBEDTLS_ENTROPY_HARDWARE_ALT)

int mbedtls_hardware_poll( void *data, unsigned char *output, size_t len, size_t *olen )
{
#if 0
    tls_crypto_random_init(tls_os_get_time(), CRYPTO_RNG_SWITCH_32);
    tls_os_time_delay(0);
    tls_crypto_random_bytes(output, len);
    tls_crypto_random_stop();
#else
	{
		extern int random_get_bytes(void *buf, size_t len);
	    random_get_bytes(output, len);
	}
#endif
    *olen = len;
    return 0;
}
#endif

mbedtls_time_t mbedtls_time( mbedtls_time_t* timer )
{
    return (mbedtls_time_t)tls_os_get_time();
}

