#ifndef CHACHA20_H
#define CHACHA20_H

#define ASSERT(expr)				((void)0)
#define KEY_SIZE                    32
#define NONCE_SIZE                  12

typedef unsigned __int64 size_t;
typedef unsigned __int8  uint8_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
typedef unsigned long DWORD;
typedef unsigned char BYTE;


typedef struct _ENCRYPTED_FILE_HEADER {
	DWORD Signature;
	BYTE Key[KEY_SIZE];
	BYTE Nonce[NONCE_SIZE];
} ENCRYPTED_FILE_HEADER, * PENCRYPTED_FILE_HEADER;


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