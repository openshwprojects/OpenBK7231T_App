/*
 * SHA1 hash implementation and interface functions
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

#ifndef SHA1_H
#define SHA1_H

#include "tls_common.h"

#define SHA1_MAC_LEN 20

struct SHA1Context {
	u32 state[5];
	u32 count[2];
	unsigned char buffer[64];
};

void SHA1Transform(u32 state[5], const unsigned char buffer[64]);
int hmac_sha1_vector(const u8 *key, size_t key_len, size_t num_elem,
		     const u8 *addr[], const size_t *len, u8 *mac);
int hmac_sha1(const u8 *key, size_t key_len, const u8 *data, size_t data_len,
	       u8 *mac);
int sha1_prf(const u8 *key, size_t key_len, const char *label,
	     const u8 *data, size_t data_len, u8 *buf, size_t buf_len);
int sha1_t_prf(const u8 *key, size_t key_len, const char *label,
	       const u8 *seed, size_t seed_len, u8 *buf, size_t buf_len);
int pbkdf2_sha1(const char *passphrase, const char *ssid, size_t ssid_len,
		int iterations, u8 *buf, size_t buflen);
int sha1_vector(size_t num_elem, const u8 *addr[], const size_t *len, u8 *mac);
void SHA1Init(struct SHA1Context* context);
void SHA1Update(struct SHA1Context* context, const void *_data, u32 len);
void SHA1Final(unsigned char digest[20], struct SHA1Context* context);
int sha1(const u8 *addr, int len, u8 *mac);

#endif /* SHA1_H */
