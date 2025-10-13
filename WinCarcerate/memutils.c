#include <windows.h>
#include "peb.h"
#include "memutils.h"


#pragma function(memset)
void* __cdecl memset(void* dest, int c, size_t count) {
    unsigned char* p = (unsigned char*)dest;
    while (count--) { *p++ = (unsigned char)c; }
    return dest;
}

static HANDLE _GetProcessHeap(void) {
    PPEB pPeb = GetPeb();
    return (HANDLE)pPeb->ProcessHeap; 
}

void* _HeapAlloc(size_t size) {
    return HeapAlloc(_GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

BOOL _HeapFree(void* mem) {
    return HeapFree(_GetProcessHeap(), 0, mem);
}

void* _memcpy(void* dest, const void* src, size_t count) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (count--) { *d++ = *s++; }
    return dest;
}
