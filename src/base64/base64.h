#ifndef _BASE64_H
#define _BASE64_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


size_t b64_encoded_size(size_t inlen);
char *b64_encode(const unsigned char *in, size_t len);
size_t b64_decoded_size(const char *in);
void b64_generate_decode_table();
int b64_isvalidchar(char c);
int b64_decode(const char *in, unsigned char *out, size_t outlen);

#endif