#include "chacha20.h"
#include "memutils.h"


static uint32_t rotl32(uint32_t x, int n) {
	return (x << n) | (x >> (32 - n));
}

static uint32_t pack4(const uint8_t* a) {
	uint32_t res = 0;

	res |= (uint32_t)a[0] << 0 * 8;
	res |= (uint32_t)a[1] << 1 * 8;
	res |= (uint32_t)a[2] << 2 * 8;
	res |= (uint32_t)a[3] << 3 * 8;

	return res;
}

static void ChaCha20_Block_Init(CHACHA20_CTX* ctx, uint8_t key[], uint8_t nonce[]) {

	_memcpy(ctx->key, key, sizeof(ctx->key));
	_memcpy(ctx->nonce, nonce, sizeof(ctx->nonce));

	const uint8_t* magic_constant = (uint8_t*)"expand 32-byte k";
	ctx->state[0] = pack4(magic_constant + 0 * 4);
	ctx->state[1] = pack4(magic_constant + 1 * 4);
	ctx->state[2] = pack4(magic_constant + 2 * 4);
	ctx->state[3] = pack4(magic_constant + 3 * 4);
	ctx->state[4] = pack4(key + 0 * 4);
	ctx->state[5] = pack4(key + 1 * 4);
	ctx->state[6] = pack4(key + 2 * 4);
	ctx->state[7] = pack4(key + 3 * 4);
	ctx->state[8] = pack4(key + 4 * 4);
	ctx->state[9] = pack4(key + 5 * 4);
	ctx->state[10] = pack4(key + 6 * 4);
	ctx->state[11] = pack4(key + 7 * 4);

	ctx->state[12] = 0;
	ctx->state[13] = pack4(nonce + 0 * 4);
	ctx->state[14] = pack4(nonce + 1 * 4);
	ctx->state[15] = pack4(nonce + 2 * 4);

	_memcpy(ctx->nonce, nonce, sizeof(ctx->nonce));
}

static void ChaCha20_Block_SetCounter(CHACHA20_CTX* ctx, uint64_t counter) {

	ctx->state[12] = (uint32_t)counter;
	ctx->state[13] = pack4(ctx->nonce + 0 * 4) + (uint32_t)(counter >> 32);
}

static void ChaCha20_Block_Next(CHACHA20_CTX* ctx) {

	for (int i = 0; i < 16; i++) ctx->keystream32[i] = ctx->state[i];

#define CHACHA20_QUARTERROUND(x, a, b, c, d) \
    x[a] += x[b]; x[d] = rotl32(x[d] ^ x[a], 16); \
    x[c] += x[d]; x[b] = rotl32(x[b] ^ x[c], 12); \
    x[a] += x[b]; x[d] = rotl32(x[d] ^ x[a], 8); \
    x[c] += x[d]; x[b] = rotl32(x[b] ^ x[c], 7);

	for (int i = 0; i < 10; i++) {
		CHACHA20_QUARTERROUND(ctx->keystream32, 0, 4, 8, 12)
			CHACHA20_QUARTERROUND(ctx->keystream32, 1, 5, 9, 13)
			CHACHA20_QUARTERROUND(ctx->keystream32, 2, 6, 10, 14)
			CHACHA20_QUARTERROUND(ctx->keystream32, 3, 7, 11, 15)
			CHACHA20_QUARTERROUND(ctx->keystream32, 0, 5, 10, 15)
			CHACHA20_QUARTERROUND(ctx->keystream32, 1, 6, 11, 12)
			CHACHA20_QUARTERROUND(ctx->keystream32, 2, 7, 8, 13)
			CHACHA20_QUARTERROUND(ctx->keystream32, 3, 4, 9, 14)
	}

	for (int i = 0; i < 16; i++) 
		ctx->keystream32[i] += ctx->state[i];

	uint32_t* counter = ctx->state + 12;
	counter[0]++;

	if (0 == counter[0]) {
		counter[1]++;
		ASSERT(0 != counter[1]);
	}
}

void ChaCha20_Ctx_Init(CHACHA20_CTX* ctx, uint8_t key[], uint8_t nonce[], uint64_t counter) {

	memset(ctx, 0, sizeof(CHACHA20_CTX));

	ChaCha20_Block_Init(ctx, key, nonce);
	ChaCha20_Block_SetCounter(ctx, counter);

	ctx->counter = counter;
	ctx->position = 64;
}

void ChaCha20_Xor(CHACHA20_CTX* ctx, uint8_t* bytes, size_t n_bytes) {
	uint8_t* keystream8 = (uint8_t*)ctx->keystream32;

	for (size_t i = 0; i < n_bytes; i++) {

		if (ctx->position >= 64) {
			ChaCha20_Block_Next(ctx);
			ctx->position = 0;
		}
		bytes[i] ^= keystream8[ctx->position];
		ctx->position++;
	}
}