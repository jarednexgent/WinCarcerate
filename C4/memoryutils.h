#pragma once
#include <Windows.h>

#ifndef MEMORY_UTILS_H
#define MEMORY_UTILS_H

void* __cdecl memset(void* Destination, int Value, size_t Size);

void* _heapalloc(size_t size);

void _heapfree(void* mem);

void* _memcpy(void* dest, const void* src, size_t count);

#endif