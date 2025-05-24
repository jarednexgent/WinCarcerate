#pragma once
#include <windows.h>

#ifndef RC4_H
#define RC4_H

typedef struct _RC4_CTX {
    unsigned int i;
    unsigned int j;
    unsigned char s[256];
} RC4_CTX, * PRC4_CTX;

void KSA(RC4_CTX* ctx, const unsigned char* key, size_t len);

void PRGA(RC4_CTX* ctx, BYTE* data, size_t len);

#endif