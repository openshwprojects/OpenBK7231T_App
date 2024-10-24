/**
 * @file aes.h
 * @brief AES functions
 * @copyright (c) 2003-2006, Jouni Malinen <jkmaline@cc.hut.fi>
 *
 * @note This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

#ifndef AES_H
#define AES_H

#include <stddef.h>
#include "wm_type_def.h"


int  aes_128_cbc_encrypt(const u8 *key, const u8 *iv, u8 *data,
				     size_t data_len);
int  aes_128_cbc_decrypt(const u8 *key, const u8 *iv, u8 *data,
				     size_t data_len);

#endif /* AES_H */

