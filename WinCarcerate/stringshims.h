#pragma once
// CRT-free wrappers using Win32 + StrSafe + Shlwapi, portable across cl.exe, clang-cl, clang/MinGW.

#ifndef STRINGSHIMS_H
#define STRINGSHIMS_H

    /* ---------------- Compiler portability ---------------- */
#if defined(_MSC_VER)
#define CC_ALWAYS_INLINE __forceinline
#define CC_MSVC 1
#else
#if defined(__GNUC__) || defined(__clang__)
#define CC_ALWAYS_INLINE inline __attribute__((always_inline))
#else
#define CC_ALWAYS_INLINE inline
#endif
#define CC_MSVC 0
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600  // Vista+ for CompareStringOrdinal 
#endif

#include <windows.h>
#include <strsafe.h>    // StringCch* (header-only wrappers)
#include <shlwapi.h>    // StrStrIW, StrRChrW 

#if CC_MSVC
    /* ------- Non-portable MSVC autolink; other compilers must pass -lShlwapi ------- */
#pragma comment(lib, "Shlwapi.lib")
#endif

    /* ---------------- Helper for Destination Length ---------------- */
#ifndef _countof
#define _countof(a) (sizeof(a) / sizeof((a)[0]))
#endif

    /* ---------------- Length ---------------- */
    CC_ALWAYS_INLINE int shim_strlenA(const char* s) { 
        return (int)lstrlenA(s); 
    }
    CC_ALWAYS_INLINE size_t shim_wcslen(const wchar_t* s) { 
        return (size_t)lstrlenW(s); 
    }

    /* ---------------- Copy / Cat (bounds-checked) ---------------- */
    CC_ALWAYS_INLINE HRESULT shim_wcscpy_s(wchar_t* d, size_t cchDest, const wchar_t* s) {
        return StringCchCopyW(d, cchDest, s);
    }
    CC_ALWAYS_INLINE HRESULT shim_wcsncpy_s(wchar_t* d, size_t cchDest, const wchar_t* s, size_t cchMax) {
        return StringCchCopyNW(d, cchDest, s, cchMax);
    }
    CC_ALWAYS_INLINE HRESULT shim_wcscat(wchar_t* d, size_t cchDest, const wchar_t* s) {
        return StringCchCatW(d, cchDest, s);
    }

    /* ---------------- Compare ---------------- */
    CC_ALWAYS_INLINE int shim_wcscmp(const wchar_t* a, const wchar_t* b) {
        return lstrcmpW(a, b);
    }
    CC_ALWAYS_INLINE int shim_wcsicmp(const wchar_t* a, const wchar_t* b) {
        return lstrcmpiW(a, b);
    }

    /* ---------------- Robust ordinal compare (returns -1/0/+1; -2 on failure) ---------------- */
    CC_ALWAYS_INLINE int shim_compare_ordinal(const wchar_t* a, int cchA, const wchar_t* b, int cchB, BOOL ignoreCase) {
        int r = CompareStringOrdinal(a, cchA, b, cchB, ignoreCase);
        return (r == CSTR_LESS_THAN) ? -1 : (r == CSTR_EQUAL) ? 0 : (r == CSTR_GREATER_THAN) ? +1 : -2;
    }
    CC_ALWAYS_INLINE int shim_wcsnicmp(const wchar_t* a, const wchar_t* b, size_t n) {
        return shim_compare_ordinal(a, (int)n, b, (int)n, TRUE);
    }

    /* ---------------- Find last char / substring ---------------- */
    CC_ALWAYS_INLINE const wchar_t* shim_wcsrchr(const wchar_t* s, const wchar_t ch) {
        return StrRChrW(s, NULL, ch);
    }
    CC_ALWAYS_INLINE const wchar_t* shim_StrStrIW(const wchar_t* hay, const wchar_t* needle) {
        return StrStrIW(hay, needle);
    }

    /* ---------------- Formatting (bounds-checked, wide) ---------------- */
    static CC_ALWAYS_INLINE int shim_vsnwprintf(wchar_t* dst, size_t cchDst, const wchar_t* fmt, va_list ap) {
        if (!dst || !fmt || cchDst == 0) return -1;
        size_t i = 0;

        for (; *fmt && i + 1 < cchDst; ++fmt) {
            if (*fmt != L'%') {            // copy literal            
                dst[i++] = *fmt;
                continue;
            }
            ++fmt;                        // past '%'
            if (*fmt == L'%') {           // escaped percent         
                dst[i++] = L'%';
                continue;
            }
            else if (*fmt == L's') {      // %s
                const wchar_t* s = va_arg(ap, const wchar_t*);

                if (!s) {
                    s = L"(null)";
                }
                while (*s && i + 1 < cchDst) {
                    dst[i++] = *s++;
                }
            }
            else if (*fmt == L'd') {      // %d
                int v = va_arg(ap, int);
                wchar_t tmp[32]; 
                int t = 0; 
                int neg = (v < 0);
                unsigned u = neg ? (unsigned)(-v) : (unsigned)v;

                do {
                    tmp[t++] = (wchar_t)(L'0' + (u % 10)); u /= 10; 
                } while (u && t < (int)_countof(tmp));

                if (neg && i + 1 < cchDst) {
                    dst[i++] = L'-';
                }
                while (t-- > 0 && i + 1 < cchDst) {
                    dst[i++] = tmp[t];
                }
            }
            else if (*fmt == L'X') {      // %X
                unsigned v = va_arg(ap, unsigned);
                wchar_t tmp[32]; 
                int t = 0;

                do {
                    unsigned d = v & 0xF;
                    tmp[t++] = (wchar_t)(d < 10 ? L'0' + d : L'A' + (d - 10));
                    v >>= 4;
                } while (v && t < (int)_countof(tmp));

                while (t-- > 0 && i + 1 < cchDst) {
                    dst[i++] = tmp[t];
                }
            }
            else {
                // Unknown specifier: render literally
                if (i + 1 < cchDst) 
                    dst[i++] = L'%';
                if (*fmt && i + 1 < cchDst) 
                    dst[i++] = *fmt;
            }
        }
        dst[i] = L'\0';
        return (int)i;
    }

    CC_ALWAYS_INLINE int shim_snwprintf(wchar_t* dst, size_t cchDst, const wchar_t* fmt, ...) {
        va_list ap; va_start(ap, fmt);
        int r = shim_vsnwprintf(dst, cchDst, fmt, ap);
        va_end(ap);
        return r;
    }



#define PRINT(STR, ... ) do {                                                               \
    LPWSTR buf = (LPWSTR)_HeapAlloc(1024);                                                  \
    if ( buf != NULL ) {                                                                    \
        int len = shim_snwprintf(buf, 1024, STR, __VA_ARGS__);                              \
        WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), buf, len, NULL, NULL);               \
        _HeapFree(buf);                                                                     \
    }                                                                                       \
} while (0)    
    

#endif // STRINGSHIMS_H
