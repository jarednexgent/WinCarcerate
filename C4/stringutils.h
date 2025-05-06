#pragma once
#include <Windows.h>

#ifndef STRING_UTILS_H
#define STRING_UTILS_H

SIZE_T WCharStringToCharString(PCHAR Destination, PWCHAR Source, SIZE_T MaximumAllowed);

char* _strncat(char* dest, const char* src, size_t count);
wchar_t* _wcscat(wchar_t* dest, const wchar_t* src);

int _strcmp(const char* string1, const char* string2);
int _wcscmp(const wchar_t* string1, const wchar_t* string2);
int _wcsicmp_(const wchar_t* s1, const wchar_t* s2);
int _wcsnicmp_(const wchar_t* s1, const wchar_t* s2, size_t count);

int _strlen(const char* string);
size_t _wcslen(const wchar_t* string);

char* _strcpy(char* dest, const char* src);
errno_t _strcpy_s(char* dest, size_t szDest, const char* src);
char* _strncpy(char* dest, const char* src, size_t count);

wchar_t* _wcsrchr(const wchar_t* string, wchar_t c);
LPCWSTR _StrStrIW(PCWSTR string, PCWSTR search);

int _snprintf_(char* buf, size_t bufSize, const char* fmt, ...);
int _swprintf_(wchar_t* buf, size_t bufSize, const wchar_t* fmt, ...);
int _wsprintfA(char* buf, const char* fmt, ...);
int _wsprintfW(wchar_t* buf, const wchar_t* fmt, ...);

#endif