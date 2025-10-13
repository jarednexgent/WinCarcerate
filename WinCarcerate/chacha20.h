// chacha20.h
#ifndef CHACHA20_H
#define CHACHA20_H

#include <inttypes.h>

#define ASSERT(expr)				((void)0)


typedef struct _CHACHA20_CTX {
	uint32_t keystream32[16];
	size_t position;
	uint8_t key[32];
	uint8_t nonce[12];
	uint64_t counter;
	uint32_t state[16];
} CHACHA20_CTX, * PCHACHA20_CTX;


void ChaCha20_Ctx_Init(CHACHA20_CTX* ctx, uint8_t key[], uint8_t nonce[], uint64_t counter);
void ChaCha20_Xor(CHACHA20_CTX* ctx, uint8_t* bytes, size_t n_bytes);


#endif // CHACHA20_H