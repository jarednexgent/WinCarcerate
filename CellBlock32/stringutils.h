// stringutils.h
#ifndef STRINGUTILS_H
#define STRINGUTILS_H


#include <Windows.h>


#define PRINT(STR, ... ) do {                                                               \
    LPWSTR buf = (LPWSTR)_heapalloc(1024);                                                  \
    if ( buf != NULL ) {                                                                    \
        int len = _wsprintfW(buf, STR, __VA_ARGS__);                                        \
        WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), buf, len, NULL, NULL);               \
        _heapfree(buf);                                                                     \
    }                                                                                       \
} while (0)                                                                                 \

                                      
int _strlen(const char* string);
size_t _wcslen(const wchar_t* string);
wchar_t* _wcscat(wchar_t* dest, const wchar_t* src);
int _wcscmp(const wchar_t* string1, const wchar_t* string2);
int __cdecl _wcsicmp(const wchar_t* string1, const wchar_t* string2);
wchar_t* _wcsrchr(const wchar_t* string, wchar_t c);
errno_t _wcscpy_s(wchar_t* dest, size_t dest_size, const wchar_t* src);
errno_t _wcsncpy_s(WCHAR * dest, size_t dest_size, const WCHAR * src, size_t count);
LPCWSTR _StrStrIW(PCWSTR string, PCWSTR search);
int _snprintfW(wchar_t* buf, size_t buf_size, const wchar_t* fmt, ...);
int _wsprintfW(wchar_t* buf, const wchar_t* fmt, ...);

#endif // STRINGUTILS_H