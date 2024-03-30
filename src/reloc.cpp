#include "reloc.h"

#include "sysconfig.h"
#include "sysdeps.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "doshunks.h"

#include "machdep/maccess.h"

static uint32_t ReadLong(ReadFunc read, void* fh) {
    uint32_t v;
    if (read(fh, &v, sizeof(v)) != sizeof(v)) {
        printf("DONE!\n");
        return 0;
    }
    v = do_byteswap_32(v);
    return v;
}

static uint32_t ReadWord(ReadFunc read, void* fh) {
    uint16_t v;
    if (read(fh, &v, sizeof(v)) != sizeof(v))
        return 0;
    v = do_byteswap_16(v);
    return v;
}

static uint32_t ReadSize(ReadFunc read, void* fh, uint32_t* memFlags) {
    uint32_t size = ReadLong(read, fh);

    uint32_t flags = size >> 29;
    if ((flags & 0x3) == 0x3) {
        flags = ReadLong(read, fh);
        flags &= 0x00ffffffff;
    }
    *memFlags = flags;

    size <<= 2;  // & 0x3fffffff * sizeof(uint32_t)
    return size;
}

static const char* ReadString(ReadFunc read, void* fh, char* buffer, uint32_t bufferLen) {
    uint32_t size = ReadLong(read, fh) * sizeof(uint32_t);
    if (!size)
        return 0;

    bufferLen &= ~0x3;  // align to longs
    uint32_t len = size < bufferLen ? size : bufferLen;
    read(fh, buffer, len);
    buffer[len - 1] = 0;

    for (size -= len; size > 0; size -= sizeof(uint32_t))
        ReadLong(read, fh);

    return buffer;
}

const char* const hunkNames[] = {
    "HUNK_UNIT",          // 999
    "HUNK_NAME",          // 1000
    "HUNK_CODE",          // 1001
    "HUNK_DATA",          // 1002
    "HUNK_BSS",           // 1003
    "HUNK_RELOC32",       // 1004
    "HUNK_RELOC16",       // 1005
    "HUNK_RELOC8",        // 1006
    "HUNK_EXT",           // 1007
    "HUNK_SYMBOL",        // 1008
    "HUNK_DEBUG",         // 1009
    "HUNK_END",           // 1010
    "HUNK_HEADER",        // 1011
    "HUNK_CONT",          // 1012
    "HUNK_OVERLAY",       // 1013
    "HUNK_BREAK",         // 1014
    "HUNK_DREL32",        // 1015
    "HUNK_DREL16",        // 1016
    "HUNK_DREL8",         // 1017
    "HUNK_LIB",           // 1018
    "HUNK_INDEX",         // 1019
    "HUNK_RELOC32SHORT",  // 1020
    "HUNK_RELRELOC32",    // 1021
    "HUNK_ABSRELOC16",    // 1022
};

#define MEMF_PUBLIC (1UL << 0)
#define MEMF_CHIP (1UL << 1)
#define MEMF_FAST (1UL << 2)
#define MEMF_LOCAL (1UL << 8)
#define MEMF_24BITDMA (1UL << 9)
#define MEMF_KICK (1UL << 10)
#define MEMF_CLEAR (1UL << 16)
#define MEMF_LARGEST (1UL << 17)
#define MEMF_REVERSE (1UL << 18)
#define MEMF_TOTAL (1UL << 19)
#define MEMF_NO_EXPUNGE (1UL << 31)

static void printfMemFlags(uint32_t v) {
    // if(!v) printf("MEMF_ANY");
    if (v & MEMF_PUBLIC) {
        v &= ~MEMF_PUBLIC;
        printf("PUBLIC %s", v ? "|" : "");
    }
    if (v & MEMF_CHIP) {
        v &= ~MEMF_CHIP;
        printf("CHIP %s", v ? "|" : "");
    }
    if (v & MEMF_FAST) {
        v &= ~MEMF_FAST;
        printf("FAST %s", v ? "|" : "");
    }
    if (v & MEMF_LOCAL) {
        v &= ~MEMF_LOCAL;
        printf("LOCAL %s", v ? "|" : "");
    }
    if (v & MEMF_24BITDMA) {
        v &= ~MEMF_24BITDMA;
        printf("24BITDMA %s", v ? "|" : "");
    }
    if (v & MEMF_KICK) {
        v &= ~MEMF_KICK;
        printf("KICK %s", v ? "|" : "");
    }
    if (v & MEMF_CLEAR) {
        v &= ~MEMF_CLEAR;
        printf("CLEAR %s", v ? "|" : "");
    }
    if (v & MEMF_LARGEST) {
        v &= ~MEMF_LARGEST;
        printf("LARGEST %s", v ? "|" : "");
    }
    if (v & MEMF_REVERSE) {
        v &= ~MEMF_REVERSE;
        printf("REVERSE %s", v ? "|" : "");
    }
    if (v & MEMF_TOTAL) {
        v &= ~MEMF_TOTAL;
        printf("TOTAL %s", v ? "|" : "");
    }
    if (v & MEMF_NO_EXPUNGE) {
        v &= ~MEMF_NO_EXPUNGE;
        printf("NO_EXPUNGE %s", v ? "|" : "");
    }
    if (v)
        printf("%08x", v);
}
#define BADDR(x) ((APTR)((uint32_t)(x) << 2))
#define MKBADDR(x) (((int)(x)) >> 2)

// Mimics dos.library/InternalLoadSeg()

BPTR CustomLoadSeg(void* fh, struct LoadSegFuncs* funcs) {
    BPTR segList;

    // Starts with 0x000003f3?
    if (ReadLong(funcs->read, fh) != HUNK_HEADER) {
        printf("not hunk format!\n");
        return 0;
    }

    printf("%s\n", hunkNames[HUNK_HEADER - HUNK_UNIT]);

    // We don't support resident lib loading
    if (ReadLong(funcs->read, fh)) {
        printf("resident lib loading not supported!\n");
        return 0;
    }

    uint32_t memFlags = 0;

    uint32_t tableSize = ReadSize(funcs->read, fh, &memFlags);
    uint32_t firstHunk = ReadLong(funcs->read, fh);
    uint32_t lastHunk = ReadLong(funcs->read, fh);

    // printf("tableSize = %d\n", tableSize);
    // printf("firstHunk = %d\n", firstHunk);
    // printf("lastHunk  = %d\n", lastHunk );
    printf("  Numhunks  = %d  (%d to %d)\n", lastHunk - firstHunk + 1, firstHunk, lastHunk);

    APTR segTable[tableSize];

    {
        APTR* currentHunk = &segTable[firstHunk];
        BPTR* nextSeg = &segList;
        for (uint32_t hunkIndex = firstHunk; hunkIndex <= lastHunk; ++hunkIndex) {
            uint32_t hunkSize = ReadSize(funcs->read, fh, &memFlags);
            printf("  Hunk %03d : %d Bytes ", hunkIndex, hunkSize);
            printfMemFlags(memFlags);
            printf("\n");

            APTR hunkPtr = funcs->alloc(hunkSize + sizeof(BPTR), memFlags);
            BPTR bptr = MKBADDR(hunkPtr);
            *currentHunk++ = hunkPtr;
            // *nextSeg = bptr;
            do_put_mem_long((uae_u32*)nextSeg, bptr);
            nextSeg = (BPTR*)funcs->map(hunkPtr);
        }
        *nextSeg = 0;
    }

    uint32_t hunkIndex = firstHunk;
    uint32_t relocIndex = hunkIndex;

    while (1) {
        uint32_t hunkType = ReadLong(funcs->read, fh);
        // printf("hunkType = %08lx\n", hunkType);

        if (!hunkType) {
            // printf("null hunkType!\n");
            break;
        }

        hunkType &= 0x3fffffff;  // is this necessary?

        if (hunkType < HUNK_UNIT || HUNK_ABSRELOC16 < hunkType) {
            if (hunkType & HUNKF_ADVISORY)
                hunkType = HUNK_DEBUG;
            else {
                printf("unknown hunk %08x!\n", hunkType);
                return 0;
            }
        }

        printf("%s", hunkNames[hunkType - HUNK_UNIT]);
        switch (hunkType) {
            case HUNK_UNIT:
            case HUNK_NAME: {
                char str[256];
                const char* ret = ReadString(funcs->read, fh, str, sizeof(str));
                if (!ret)
                    break;
                printf("\t'%s'\n", ret);
                break;
            }

            case HUNK_CODE:
            case HUNK_DATA:
            case HUNK_BSS: {
                // printf("HUNK #%d\n", hunkIndex);
                uint32_t size = ReadSize(funcs->read, fh, &memFlags);

                printf("\t%d Bytes\n", size);

                relocIndex = hunkIndex;

                uint32_t hunkAddr = segTable[hunkIndex];
                printf(" @ %08x\n", hunkAddr);
                uint32_t* currentHunk = (uint32_t*)funcs->map(segTable[hunkIndex++] + 4);
                if (hunkType != HUNK_BSS) {
                    if (funcs->read(fh, currentHunk, size) != size) {
                        printf("failed loading hunk!!\n");
                        return 0;
                    }
                } else {
                    // TODO always clear the rest of the hunk..
                    for (uint32_t i = 0; i < size / 4; ++i) {
                        *currentHunk++ = 0;
                    }
                }
                break;
            }

            case HUNK_RELOC32: {
                /*
                32-bit absolute reloc.
                struct
                {
                    uint32_t number_of_relocs;
                    uint32_t referenced_hunk;
                    uint32_t offsets_in_this_hunk[number_of_relocs];
                }
                */
                printf("\n");

                uint8_t* currentHunk = (uint8_t*)funcs->map(segTable[relocIndex] + 4);

                while (1) {
                    uint32_t size = ReadSize(funcs->read, fh, &memFlags);
                    if (!size)
                        break;
                    size >>= 2;
                    uint32_t hunkNr = ReadLong(funcs->read, fh);
                    // printf("hunkNr = %d\n", hunkNr);
                    // $TODO sanity check hunk number!
                    uint32_t baseAddress = (uint32_t)(segTable[hunkNr] + 4);

                    while (size--) {
                        uint32_t offset = ReadLong(funcs->read, fh);
                        uint32_t* reloc = (uint32_t*)(&currentHunk[offset]);
                        // *reloc += baseAddress;
                        do_put_mem_long(reloc, do_get_mem_long(reloc) + baseAddress);
                    }
                }

                break;
            }

            case HUNK_DREL32:
            case HUNK_RELOC32SHORT: {
                /*
                32-bit absolute reloc w/ 16-bit data
                struct
                {
                    uint16_t number_of_relocs;
                    uint16_t referenced_hunk;
                    uint16_t offsets_in_this_hunk[number_of_relocs];
                }
                */

                /*
                 * Note: V37 LoadSeg uses 1015 (HUNK_DREL32) by mistake.  This will continue
                 * to be supported in future versions, since HUNK_DREL32 is illegal in load files
                 * anyways.  Future versions will support both 1015 and 1020, though anything
                 * that should be usable under V37 should use 1015.
                 */
                printf("\n");

                uint8_t* currentHunk = (uint8_t*)funcs->map(segTable[relocIndex] + 4);

                uint32_t wordsRead = 0;
                while (1) {
                    uint32_t size = ReadWord(funcs->read, fh);
                    ++wordsRead;
                    if (!size)
                        break;
                    uint32_t hunkNr = ReadWord(funcs->read, fh);
                    ++wordsRead;
                    // printf("hunkNr = %d\n", hunkNr);
                    // $TODO sanity check hunk number!
                    uint32_t baseAddress = (uint32_t)(segTable[hunkNr] + 4);
                    while (size--) {
                        uint32_t offset = ReadWord(funcs->read, fh);
                        ++wordsRead;
                        uint32_t* reloc = (uint32_t*)(&currentHunk[offset]);
                        // *reloc += baseAddress;
                        do_put_mem_long(reloc, do_get_mem_long(reloc) + baseAddress);
                    }
                }
                // if read an odd number of words, there is padding to long
                if (wordsRead & 1) {
                    ReadWord(funcs->read, fh);
                }

                break;
            }

            case HUNK_RELRELOC32: {
                /*
                32-bit pc-relative reloc w/ 16-bit data
                struct
                {
                    uint16_t number_of_relocs;
                    uint16_t referenced_hunk;
                    uint16_t offsets_in_this_hunk[number_of_relocs];
                }
                */
                printf("\n");

                uint8_t* currentHunk = (uint8_t*)funcs->map(segTable[relocIndex] + 4);

                uint32_t wordsRead = 0;
                while (1) {
                    uint32_t size = ReadWord(funcs->read, fh);
                    ++wordsRead;
                    if (!size)
                        break;
                    uint32_t hunkNr = ReadWord(funcs->read, fh);
                    ++wordsRead;
                    // printf("hunkNr = %d\n", hunkNr);
                    // $TODO sanity check hunk number!
                    uint32_t baseAddress = (uint32_t)(segTable[hunkNr] + 4);
                    while (size--) {
                        uint32_t offset = ReadWord(funcs->read, fh);
                        ++wordsRead;
                        uint32_t* reloc = (uint32_t*)(&currentHunk[offset]);
                        // *reloc -= (uint32_t)reloc;
                        // *reloc += baseAddress;
                        do_put_mem_long(reloc, do_get_mem_long(reloc) + baseAddress);
                    }
                }
                // if read an odd number of words, there is padding to long
                if (wordsRead & 1) {
                    ReadWord(funcs->read, fh);
                }

                break;
            }
            case HUNK_END:
                printf("\n\n");
                break;

            case HUNK_SYMBOL: {
                printf("\n");
                char str[256];
                while (1) {
                    const char* symbol = ReadString(funcs->read, fh, str, sizeof(str));
                    if (!symbol)
                        break;
                    uint32_t value = ReadLong(funcs->read, fh);
                    (void)value;
                    printf("            %s = $%08x\n", symbol, value);
                }
                break;
            }

            case HUNK_RELOC16:
            case HUNK_RELOC8:
            case HUNK_DREL16:
            case HUNK_DREL8: {
                printf("DRELOCs not supported\n");
                return 0;
            }

            case HUNK_HEADER:
            case HUNK_EXT:
            case HUNK_LIB:
            case HUNK_INDEX:
            case HUNK_OVERLAY:
            case HUNK_BREAK: {
                printf("Unsupported hunk (%x)\n", hunkType);
                return 0;
            }

            case HUNK_DEBUG:
            default: {
                printf(" (this could take a while.. )\n");
                uint32_t size = ReadSize(funcs->read, fh, &memFlags);
                //			funcs->read(fh, )
                size >>= 2;
                while (size--) {
                    // printf("size = %d  => ", size);
                    // ReadLong(funcs->read, fh);

                    uint32_t dummy;
                    if (funcs->read(fh, &dummy, sizeof(dummy)) != sizeof(dummy)) {
                        printf("short read ; size = %d\n", size);
                        return 0;
                    }
                }

                break;
            }
        }
    }

    return segList;
}
