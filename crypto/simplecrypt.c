#include "simplecrypt.h"
#include <string.h>

void simplecrypt_encrypt(const char *key, const char *in, char *out, size_t len)
{
    size_t keylen = strlen(key);
    for (size_t i = 0; i < len; ++i)
        out[i] = in[i] ^ key[i % keylen];
}

void simplecrypt_decrypt(const char *key, const char *in, char *out, size_t len)
{
    simplecrypt_encrypt(key, in, out, len);
}
