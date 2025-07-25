// memutils.h
#ifndef MEMUTILS_H
#define MEMUTILS_H

typedef unsigned __int64 size_t;

void* __cdecl memset(void* Destination, int Value, size_t Size);
void* _heapalloc(size_t size);
void _heapfree(void* mem);
void* _memcpy(void* dest, const void* src, size_t count);

#endif // MEMUTILS_H