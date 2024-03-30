#pragma once
#ifndef RELOC_H
#define RELOC_H

#include <stdint.h>

typedef uint32_t APTR;
typedef int BPTR;

typedef uint32_t (*ReadFunc)(void* readhandle, void* buffer, uint32_t length);
typedef APTR (*AllocAmigaFunc)(uint32_t size, uint32_t flags);
typedef void* (*MapToRealFunc)(APTR addr);

struct LoadSegFuncs {
    ReadFunc read;
    AllocAmigaFunc alloc;
    MapToRealFunc map;
};

BPTR CustomLoadSeg(void* fh, struct LoadSegFuncs* funcs);

#endif
