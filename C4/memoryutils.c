#include <Windows.h>
#include "structs.h"
#include "memoryutils.h"


void __chkstk(void) {
    // do nothing (not needed for small stack frames)
}

#pragma intrinsic(memset)
#pragma function(memset)
void* __cdecl memset(void* Destination, int Value, size_t Size) {
    unsigned char* p = (unsigned char*)Destination;
    while (Size > 0) {
        *p = (unsigned char)Value;
        p++;
        Size--;
    }
    return Destination;
}

static HANDLE _GetProcessHeap() {
	return (HANDLE)((PPEB)__readgsqword(0x60))->ProcessHeap;
}

void* _heapalloc(size_t size) {
	return HeapAlloc(_GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

void _heapfree(void* mem) {
	HeapFree(_GetProcessHeap(), 0x00, mem);
}

void* _memcpy(void* dest, const void* src, size_t count) {
	char* dest2 = (char*)dest;
	const char* src2 = (const char*)src;
	while (count--)
		*dest2++ = *src2++;
	return dest;
}

int _memcmp(const void* buf1, const void* buf2, size_t count) {
	const unsigned char* buf1_2 = (const unsigned char*)buf1;
	const unsigned char* buf2_2 = (const unsigned char*)buf2;
	while (count--) {
		if (*buf1_2 != *buf2_2)
			return *buf1_2 - *buf2_2;
		++buf1_2;
		++buf2_2;
	}
	return 0;
}


