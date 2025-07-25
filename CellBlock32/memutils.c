#include <Windows.h>
#include "structs.h"
#include "memutils.h"

#pragma function(memset)
void* __cdecl memset(void* dest, int c, size_t count) {
    unsigned char* p = (unsigned char*)dest;
    while (count > 0) { *p = (unsigned char)c; p++; count--; }
    return dest;
}

static HANDLE _GetProcessHeap(void) {
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
	while (count--) { *dest2++ = *src2++; }
	return dest;
}

