#ifndef RC4_H
#define RC4_H
#include "wm_crypto_hard.h"

void Arc4Init(psCipherContext_t *ctx, unsigned char *key, uint32 keylen);
int32 Arc4_skip(psCipherContext_t *ctx, unsigned char *in,
			   unsigned char *out, size_t skip, uint32 len);
/**
 * rc4_skip - XOR RC4 stream to given data with skip-stream-start
 * @key: RC4 key
 * @keylen: RC4 key length
 * @skip: number of bytes to skip from the beginning of the RC4 stream
 * @data: data to be XOR'ed with RC4 stream
 * @data_len: buf length
 * Returns: 0 on success, -1 on failure
 *
 * Generate RC4 pseudo random stream for the given key, skip beginning of the
 * stream, and XOR the end result with the data buffer to perform RC4
 * encryption/decryption.
 */
int rc4_skip(const u8 *key, size_t keylen, size_t skip,
	     u8 *data, size_t data_len);
/**
 * rc4 - XOR RC4 stream to given data with skip-stream-start
 * @key: RC4 key
 * @keylen: RC4 key length
 * @data: data to be XOR'ed with RC4 stream
 * @data_len: buf length
 * Returns: 0 on success, -1 on failure
 *
 * Generate RC4 pseudo random stream for the given key, skip beginning of the
 * stream, and XOR the end result with the data buffer to perform RC4
 * encryption/decryption.
 */

int rc4(const u8 *key, size_t keylen, u8 *data, size_t data_len);

#endif /* end of RC4_H */
