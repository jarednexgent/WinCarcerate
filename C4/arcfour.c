#include <Windows.h>
#include "arcfour.h"


void KSA(RC4_CTX* ctx, const unsigned char* key, size_t len) {
    ctx->i = ctx->j = 0;
    for (int i = 0; i < 256; i++) ctx->s[i] = i;
    for (int i = 0, j = 0; i < 256; i++) {
        j = (j + ctx->s[i] + key[i % len]) % 256;
        BYTE tmp = ctx->s[i];
        ctx->s[i] = ctx->s[j];
        ctx->s[j] = tmp;
    }
}

void PRGA(RC4_CTX* ctx, BYTE* data, size_t len) {
    unsigned int i = ctx->i, j = ctx->j;
    BYTE tmp;
    for (size_t k = 0; k < len; k++) {
        i = (i + 1) % 256;
        j = (j + ctx->s[i]) % 256;
        tmp = ctx->s[i];
        ctx->s[i] = ctx->s[j];
        ctx->s[j] = tmp;
        data[k] ^= ctx->s[(ctx->s[i] + ctx->s[j]) % 256];
    }
    ctx->i = i;
    ctx->j = j;
}