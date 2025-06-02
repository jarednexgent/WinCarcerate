#include <stdarg.h>
#include "stringutils.h"

// string concatenate - WIDE
wchar_t* _wcscat(wchar_t* dest, const wchar_t* src) {
	wchar_t* dest2 = dest;
	while (*dest2)
		++dest2;
	while (*src)
		*dest2++ = *src++;
	*dest2 = 0;
	return dest;
}

// string compare - WIDE
int _wcscmp(const wchar_t* string1, const wchar_t* string2) {
	while (*string1 && *string1 == *string2) {
		++string1;
		++string2;
	}
	return *(wchar_t*)string1 - *(wchar_t*)string2;
}

// string length - ASCII
int _strlen(const char* string) {
	const char* string2;
	for (string2 = string; *string2; ++string2);
	return (int)(string2 - string);
}

// string length - WIDE
size_t _wcslen(const wchar_t* string) {
	const wchar_t* string2 = string;
	while (*string2) ++string2;
	return (size_t)(string2 - string);
}


// secure string copy - WIDE
errno_t _wcscpy_s(wchar_t* dest, size_t szDest, const wchar_t* src) {
	if (!dest || !src || szDest == 0)
		return EINVAL;  

	wchar_t* dest2 = dest;
	size_t remaining = szDest;

	while (*src) {
		if (remaining <= 1) {
			*dest = '\0';  
			return ERANGE; 
		}
		*dest2++ = *src++;
		remaining--;
	}
	*dest2 = '\0';
	return 0; 
}


// secure string copy 'count' number of characters - WIDE
errno_t _wcsncpy_s(WCHAR* dest, size_t destSize, const WCHAR* src, size_t count) {
	if (!dest || !src || destSize == 0)
		return EINVAL;

	size_t copied = 0;
	while (copied < count && *src) {
		if (copied >= destSize - 1)
			return ERANGE;
		dest[copied++] = *src++;
	}

	if (copied < destSize)
		dest[copied] = L'\0';
	else
		return ERANGE;

	return 0;
}

// find the last occurrence of a character in a string - WIDE)
wchar_t* _wcsrchr(const wchar_t* string, wchar_t c) {
	const wchar_t* string2 = string;
	while (*string2)
		++string2;
	while (string2 != string && *string2 != c)
		--string2;
	if (*string2 == c)
		return (wchar_t*)string2;
	return NULL;
}


// full case-insensitive string comparison - WIDE
int _stricmpW(const wchar_t* s1, const wchar_t* s2) {
	wchar_t c1, c2;

	while (*s1 && *s2) {
		c1 = *s1++;
		c2 = *s2++;

		if (c1 >= L'A' && c1 <= L'Z') c1 += 32;
		if (c2 >= L'A' && c2 <= L'Z') c2 += 32;

		if (c1 != c2)
			return (int)(c1 - c2);
	}

	return (int)(*s1 - *s2);
}

// Partial case-sensitive substring comparison - WIDE
static inline int _wcsnicmp(const wchar_t* s1, const wchar_t* s2, size_t count) {
	wchar_t c1, c2;

	while (count-- > 0) {
		c1 = *s1++;
		c2 = *s2++;

		if (c1 >= L'A' && c1 <= L'Z') c1 += 32;
		if (c2 >= L'A' && c2 <= L'Z') c2 += 32;

		if (c1 != c2)
			return (int)(c1 - c2);
		if (c1 == L'\0')
			break;
	}

	return 0;
}

// Case-insensitive substring search - WIDE
LPCWSTR _StrStrIW(PCWSTR string, PCWSTR search) {
	if (!string || !search) return NULL;

	size_t strlen = _wcslen(search);
	while (*string) {
		if (_wcsnicmp(string, search, strlen) == 0)
			return string;
		string++;
	}
	return NULL;
}

// custom string formatter - WIDE
int _snprintfW(wchar_t* buf, size_t bufSize, const wchar_t* fmt, ...) {
	if (!buf || !fmt || bufSize == 0) return -1;

	va_list args;
	va_start(args, fmt); 

	size_t len = 0; 

	while (*fmt && len < bufSize - 1) { 
		if (*fmt == '%') {
			fmt++; 

			if (*fmt == 's') {				
				wchar_t* s = va_arg(args, wchar_t*);
				while (*s && len < bufSize - 1)
					buf[len++] = *s++;
			}
			else if (*fmt == 'd') {
				int val = va_arg(args, int);
				wchar_t tmp[16];
				int tmpLen = 0;

				if (val < 0) {
					buf[len++] = '-';
					val = -val;
				}
				do {
					tmp[tmpLen++] = '0' + (val % 10);
					val /= 10;
				} while (val && tmpLen < sizeof(tmp));

				while (tmpLen-- && len < bufSize - 1)
					buf[len++] = tmp[tmpLen];
			}
			else if (*fmt == 'X') {
				unsigned int val = va_arg(args, unsigned int);
				char tmp[16];
				int tmpLen = 0;
				do {
					int digit = (val % 16);
					tmp[tmpLen++] = (digit < 10) ? '0' + digit : 'A' + (digit - 10);
					val /= 16;
				} while (val && tmpLen < sizeof(tmp));

				while (tmpLen-- && len < bufSize - 1)
					buf[len++] = tmp[tmpLen];
			}
			else {
				buf[len++] = '%';
				if (len < bufSize - 1)
					buf[len++] = *fmt;
			}
		}
		else {
			buf[len++] = *fmt;
		}

		fmt++; 
	}

	buf[len] = '\0';
	va_end(args);
	return (int)len;
}

// Minimal custom string formatter (supports only: %s, %d, %X) - WIDE
int _wsprintfW(wchar_t* buf, const wchar_t* fmt, ...) {
	if (!buf || !fmt) return -1;

	va_list args;
	va_start(args, fmt);

	size_t len = 0;

	while (*fmt) {
		if (*fmt == L'%') {
			fmt++;  // Move past '%'

			if (*fmt == L's') {
				// Wide string argument
				wchar_t* s = va_arg(args, wchar_t*);
				while (*s) {
					buf[len++] = *s++;
				}
			}
			else if (*fmt == L'd') {
				// Integer
				int val = va_arg(args, int);
				wchar_t tmp[16];
				int tmpLen = 0;
				BOOL neg = FALSE;

				if (val < 0) {
					neg = TRUE;
					val = -val;
				}

				do {
					tmp[tmpLen++] = L'0' + (val % 10);
					val /= 10;
				} while (val);

				if (neg) buf[len++] = L'-';
				while (tmpLen--) buf[len++] = tmp[tmpLen];
			}
			else if (*fmt == L'X') {
				// Hex (uppercase)
				unsigned int val = va_arg(args, unsigned int);
				wchar_t tmp[16];
				int tmpLen = 0;

				do {
					int digit = val % 16;
					tmp[tmpLen++] = (digit < 10) ? L'0' + digit : L'A' + (digit - 10);
					val /= 16;
				} while (val);

				while (tmpLen--) buf[len++] = tmp[tmpLen];
			}
			else {
				// Unknown specifier - write raw
				buf[len++] = L'%';
				buf[len++] = *fmt;
			}
		}
		else {
			// Normal character
			buf[len++] = *fmt;
		}
		fmt++;
	}
	buf[len] = L'\0';
	va_end(args);
	return (int)len;
}
