// memutils.h
#ifndef MEMUTILS_H
#define MEMUTILS_H

#include <windows.h>
#include <stdint.h>


#define HEAP_FREE(PTR)                               \
    do {                                             \
        if ((PTR) != NULL) {                         \
            _HeapFree((PTR));                        \
            (PTR) = NULL;                            \
        }                                            \
    } while (0)


void* __cdecl memset(void* Destination, int Value, size_t Size);
void* _HeapAlloc(size_t size);
BOOL _HeapFree(void* mem);
void* _memcpy(void* dest, const void* src, size_t count);

#endif // MEMUTILS_H