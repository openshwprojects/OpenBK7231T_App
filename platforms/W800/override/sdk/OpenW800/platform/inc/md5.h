/*
 * MD5 hash implementation and interface functions
 * Copyright (c) 2003-2009, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

#ifndef MD5_H
#define MD5_H

#include "wm_type_def.h"
//typedef unsigned int size_t;
#define MD5_MAC_LEN 16

struct MD5Context {
	u32 buf[4];
	u32 bits[2];
	u8 in[64];
};

void MD5Init(struct MD5Context *context);
void MD5Update(struct MD5Context *context, unsigned char const *buf,
	       unsigned len);
void MD5Final(unsigned char digest[16], struct MD5Context *context);

int hmac_md5_vector(const u8 *key, size_t key_len, size_t num_elem,
		    const u8 *addr[], const size_t *len, u8 *mac);
int hmac_md5(const u8 *key, size_t key_len, const u8 *data, size_t data_len,
	     u8 *mac);
int md5_vector(size_t num_elem, const u8 *addr[], const size_t *len, u8 *mac);
int md5(const u8 *addr, int len, u8 *mac);

#ifdef CONFIG_FIPS
int hmac_md5_vector_non_fips_allow(const u8 *key, size_t key_len,
				   size_t num_elem, const u8 *addr[],
				   const size_t *len, u8 *mac);
int hmac_md5_non_fips_allow(const u8 *key, size_t key_len, const u8 *data,
			    size_t data_len, u8 *mac);
#else /* CONFIG_FIPS */
#define hmac_md5_vector_non_fips_allow hmac_md5_vector
#define hmac_md5_non_fips_allow hmac_md5
#endif /* CONFIG_FIPS */

#endif /* MD5_H */

