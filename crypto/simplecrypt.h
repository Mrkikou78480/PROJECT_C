#ifndef SIMPLECRYPT_H
#define SIMPLECRYPT_H

#include <stddef.h>

void simplecrypt_encrypt(const char *key, const char *in, char *out, size_t len);
void simplecrypt_decrypt(const char *key, const char *in, char *out, size_t len);

#endif
