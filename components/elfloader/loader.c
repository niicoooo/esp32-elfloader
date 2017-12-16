/*
 * A elf module loader for esp32
 *
 * Author: niicoooo <1niicoooo1@gmail.com>
 * Copyright (C) 2017 by niicoooo <1niicoooo1@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*
 * Based on the work of the elf-loader project
 * https://github.com/embedded2014/elf-loader
 * Copyright (C) 2013 Martin Ribelotta (martinribelott@gmail.com) Licensed under GNU GPL v2 or later
 * Modified by Jim Huang (jserv.tw@gmail.com)
 */


#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "loader.h"
#include "elf.h"
#include "unaligned.h"


#if INTERFACE
#include <stdio.h>
#include <stdint.h>

#ifdef __linux__
#define LOADER_FD_T FILE *
#else
#define LOADER_FD_T void*
#endif

typedef struct {
    const char *name; /*!< Name of symbol */
    void *ptr; /*!< Pointer of symbol in memory */
} ELFLoaderSymbol_t;

typedef struct {
    const ELFLoaderSymbol_t *exported; /*!< Pointer to exported symbols array */
    unsigned int exported_size; /*!< Elements on exported symbol array */
} ELFLoaderEnv_t;

typedef struct ELFLoaderContext_t ELFLoaderContext_t;

#endif


#ifdef __linux__

#include <malloc.h>
#define LOADER_ALLOC_EXEC(size) memalign(4, size)
#define LOADER_ALLOC_DATA(size) memalign(4, size)

#define MSG(...) printf(__VA_ARGS__); printf("\n");
//#define ERR(...) printf(__VA_ARGS__); printf("\n"); assert(0);
#define ERR(...) printf(__VA_ARGS__); printf("\n");

#define LOADER_GETDATA(ctx, off, buffer, size) \
    if(fseek(ctx->fd, off, SEEK_SET) != 0) { assert(0); goto err; }\
    if(fread(buffer, 1, size, ctx->fd) != size) { assert(0); goto err; }

#else

static const char* TAG = "elfLoader";
#define MSG(...) ESP_LOGI(TAG,  __VA_ARGS__);
#define ERR(...) ESP_LOGE(TAG,  __VA_ARGS__);

#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#define LOADER_ALLOC_EXEC(size) heap_caps_malloc(size, MALLOC_CAP_EXEC | MALLOC_CAP_32BIT)
#define LOADER_ALLOC_DATA(size) heap_caps_malloc(size, MALLOC_CAP_8BIT)

#define LOADER_GETDATA(ctx, off, buffer, size) \
	unalignedCpy(buffer, ctx->fd + off, size);

#endif

typedef struct ELFLoaderSection_t {
    void *data;
    int secIdx;
    size_t size;
    off_t relSecIdx;
    struct ELFLoaderSection_t* next;
} ELFLoaderSection_t;

struct ELFLoaderContext_t {
    LOADER_FD_T fd;
    void* exec;
    void* text;
    const ELFLoaderEnv_t *env;

    size_t e_shnum;
    off_t e_shoff;
    off_t shstrtab_offset;

    size_t symtab_count;
    off_t symtab_offset;
    off_t strtab_offset;

    ELFLoaderSection_t* section;
};


/*** Read data functions ***/


static int readSection(ELFLoaderContext_t *ctx, int n, Elf32_Shdr *h, char *name, size_t name_len) {
    off_t offset = ctx->e_shoff + n * sizeof(Elf32_Shdr);
    LOADER_GETDATA(ctx, offset, h, sizeof(Elf32_Shdr));

    if (h->sh_name) {
        offset = ctx->shstrtab_offset + h->sh_name;
        LOADER_GETDATA(ctx, offset, name, name_len);
    }
    return 0;
#ifdef __linux
err:
    return -1;
#endif
}

static int readSymbol(ELFLoaderContext_t *ctx, int n, Elf32_Sym *sym, char *name, size_t nlen) {
    off_t pos = ctx->symtab_offset + n * sizeof(Elf32_Sym);
    LOADER_GETDATA(ctx, pos, sym, sizeof(Elf32_Sym))
    if (sym->st_name) {
        off_t offset = ctx->strtab_offset + sym->st_name;
        LOADER_GETDATA(ctx, offset, name, nlen);
    } else {
        Elf32_Shdr shdr;
        return readSection(ctx, sym->st_shndx, &shdr, name, nlen);
    }
    return 0;
#ifdef __linux
err:
    return -1;
#endif
}


/*** Relocation functions ***/


static const char *type2String(int symt) {
#define STRCASE(name) case name: return #name;
    switch (symt) {
        STRCASE(R_XTENSA_NONE)
        STRCASE(R_XTENSA_32)
        STRCASE(R_XTENSA_ASM_EXPAND)
        STRCASE(R_XTENSA_SLOT0_OP)
    default:
        return "R_<unknow>";
    }
#undef STRCASE
}


static int relocateSymbol(Elf32_Addr relAddr, int type, Elf32_Addr symAddr, Elf32_Addr defAddr, uint32_t* from, uint32_t* to) {
    if (symAddr == 0xffffffff) {
        if (defAddr == 0x00000000) {
            ERR("Relocation: undefined symAddr");
            return -1;
        } else {
            symAddr = defAddr;
        }
    }
    switch (type) {
    case R_XTENSA_32: {
        *from = unalignedGet32((void*)relAddr);
        *to  = symAddr + *from;
        unalignedSet32((void*)relAddr, *to);
        break;
    }
    case R_XTENSA_SLOT0_OP: {
        uint32_t v = unalignedGet32((void*)relAddr);
        *from = v;

        /* *** Format: L32R *** */
        if ((v & 0x00000F) == 0x000001) {
            int32_t delta =  symAddr - ((relAddr + 3) & 0xfffffffc);
            if (delta & 0x0000003) {
                ERR("Relocation: L32R error");
                return -1;
            }
            delta =  delta >> 2;
            unalignedSet8((void*)(relAddr + 1), ((uint8_t*)&delta)[0]);
            unalignedSet8((void*)(relAddr + 2), ((uint8_t*)&delta)[1]);
            *to = unalignedGet32((void*)relAddr);
            break;
        }

        /* *** Format: CALL *** */
        /* *** CALL0, CALL4, CALL8, CALL12, J *** */
        if ((v & 0x00000F) == 0x000005) {
            int32_t delta =  symAddr - ((relAddr + 4) & 0xfffffffc);
            if (delta & 0x0000003) {
                ERR("Relocation: CALL error");
                return -1;
            }
            delta =  delta >> 2;
            delta =  delta << 6;
            delta |= unalignedGet8((void*)(relAddr + 0));
            unalignedSet8((void*)(relAddr + 0), ((uint8_t*)&delta)[0]);
            unalignedSet8((void*)(relAddr + 1), ((uint8_t*)&delta)[1]);
            unalignedSet8((void*)(relAddr + 2), ((uint8_t*)&delta)[2]);
            *to = unalignedGet32((void*)relAddr);
            break;
        }

        /* *** J *** */
        if ((v & 0x00003F) == 0x000006) {
            int32_t delta =  symAddr - (relAddr + 4);
            delta =  delta << 6;
            delta |= unalignedGet8((void*)(relAddr + 0));
            unalignedSet8((void*)(relAddr + 0), ((uint8_t*)&delta)[0]);
            unalignedSet8((void*)(relAddr + 1), ((uint8_t*)&delta)[1]);
            unalignedSet8((void*)(relAddr + 2), ((uint8_t*)&delta)[2]);
            *to = unalignedGet32((void*)relAddr);
            break;
        }

        /* *** Format: BRI8  *** */
        /* *** BALL, BANY, BBC, BBCI, BBCI.L, BBS,  BBSI, BBSI.L, BEQ, BGE,  BGEU, BLT, BLTU, BNALL, BNE,  BNONE, LOOP,  *** */
        /* *** BEQI, BF, BGEI, BGEUI, BLTI, BLTUI, BNEI,  BT, LOOPGTZ, LOOPNEZ *** */
        if (((v & 0x00000F) == 0x000007) || ((v & 0x00003F) == 0x000026) ||  ((v & 0x00003F) == 0x000036 && (v & 0x0000FF) != 0x000036)) {
            int32_t delta =  symAddr - (relAddr + 4);
            unalignedSet8((void*)(relAddr + 2), ((uint8_t*)&delta)[0]);
            *to = unalignedGet32((void*)relAddr);
            if ((delta < - (1 << 7)) || (delta >= (1 << 7))) {
                ERR("Relocation: BRI8 out of range");
                return -1;
            }
            break;
        }

        /* *** Format: BRI12 *** */
        /* *** BEQZ, BGEZ, BLTZ, BNEZ *** */
        if ((v & 0x00003F) == 0x000016) {
            int32_t delta =  symAddr - (relAddr + 4);
            delta =  delta << 4;
            delta |=  unalignedGet32((void*)(relAddr + 1));
            unalignedSet8((void*)(relAddr + 1), ((uint8_t*)&delta)[0]);
            unalignedSet8((void*)(relAddr + 2), ((uint8_t*)&delta)[1]);
            *to = unalignedGet32((void*)relAddr);
            delta =  symAddr - (relAddr + 4);
            if ((delta < - (1 << 11)) || (delta >= (1 << 11))) {
                ERR("Relocation: BRI12 out of range");
                return -1;
            }
            break;
        }

        /* *** Format: RI6  *** */
        /* *** BEQZ.N, BNEZ.N *** */
        if ((v & 0x008F) == 0x008C) {
            int32_t delta =  symAddr - (relAddr + 4);
            int32_t d2 = delta & 0x30;
            int32_t d1 = (delta << 4) & 0xf0;
            d2 |=  unalignedGet32((void*)(relAddr + 0));
            d1 |=  unalignedGet32((void*)(relAddr + 1));
            unalignedSet8((void*)(relAddr + 0), ((uint8_t*)&d2)[0]);
            unalignedSet8((void*)(relAddr + 1), ((uint8_t*)&d1)[0]);
            *to = unalignedGet32((void*)relAddr);
            if ((delta < 0) || (delta > 0x111111)) {
                ERR("Relocation: RI6 out of range");
                return -1;
            }
            break;
        }

        ERR("Relocation: unknown opcode %08X", v);
        return -1;
        break;
    }
    case R_XTENSA_ASM_EXPAND: {
        *from = unalignedGet32((void*)relAddr);
        *to = unalignedGet32((void*)relAddr);
        break;
    }
    default:
        MSG("Relocation: undefined relocation %d %s", type, type2String(type));
        assert(0);
        return -1;
    }
    return 0;
}


static ELFLoaderSection_t *findSection(ELFLoaderContext_t* ctx, int index) {
    for (ELFLoaderSection_t* section = ctx->section; section != NULL; section = section->next) {
        if (section->secIdx == index) {
            return section;
        }
    }
    return NULL;
}


static Elf32_Addr findSymAddr(ELFLoaderContext_t* ctx, Elf32_Sym *sym, const char *sName) {
    for (int i = 0; i < ctx->env->exported_size; i++) {
        if (strcmp(ctx->env->exported[i].name, sName) == 0) {
            return (Elf32_Addr)(ctx->env->exported[i].ptr);
        }
    }
    ELFLoaderSection_t *symSec = findSection(ctx, sym->st_shndx);
    if (symSec)
        return ((Elf32_Addr) symSec->data) + sym->st_value;
    return 0xffffffff;
}


static int relocateSection(ELFLoaderContext_t *ctx, ELFLoaderSection_t *s) {
    char name[33] = "<unamed>";
    Elf32_Shdr sectHdr;
    if (readSection(ctx, s->relSecIdx, &sectHdr, name, sizeof(name)) != 0) {
        ERR("Error reading section header");
        return -1;
    }
    if (!(s->relSecIdx)) {
        MSG("  Section %s: no relocation index", name);
        return 0;
    }
    if (!(s->data)) {
        ERR("Section not loaded: %s", name);
        return -1;
    }

    MSG("  Section %s", name);
    int r = 0;
    Elf32_Rela rel;
    size_t relEntries = sectHdr.sh_size / sizeof(rel);
    MSG("  Offset   Sym  Type                      relAddr  symAddr  defValue                    Name + addend");
    for (size_t relCount = 0; relCount < relEntries; relCount++) {
        LOADER_GETDATA(ctx, sectHdr.sh_offset + relCount * (sizeof(rel)), &rel, sizeof(rel))
        Elf32_Sym sym;
        char name[33] = "<unnamed>";
        int symEntry = ELF32_R_SYM(rel.r_info);
        int relType = ELF32_R_TYPE(rel.r_info);
        Elf32_Addr relAddr = ((Elf32_Addr) s->data) + rel.r_offset;		// data to be updated adress
        readSymbol(ctx, symEntry, &sym, name, sizeof(name));
        Elf32_Addr symAddr = findSymAddr(ctx, &sym, name) + rel.r_addend;								// target symbol adress
        uint32_t from, to;
        if (relType == R_XTENSA_NONE || relType == R_XTENSA_ASM_EXPAND) {
//            MSG("  %08X %04X %04X %-20s %08X          %08X                    %s + %X", rel.r_offset, symEntry, relType, type2String(relType), relAddr, sym.st_value, name, rel.r_addend);
        } else if ((symAddr == 0xffffffff) && (sym.st_value == 0x00000000)) {
            ERR("Relocation - undefined symAddr: %s", name);
            MSG("  %08X %04X %04X %-20s %08X %08X %08X                    %s + %X", rel.r_offset, symEntry, relType, type2String(relType), relAddr, symAddr, sym.st_value, name, rel.r_addend);
            r = -1;
        } else if(relocateSymbol(relAddr, relType, symAddr, sym.st_value, &from, &to) != 0) {
            ERR("  %08X %04X %04X %-20s %08X %08X %08X %08X->%08X %s + %X", rel.r_offset, symEntry, relType, type2String(relType), relAddr, symAddr, sym.st_value, from, to, name, rel.r_addend);
            r = -1;
        } else {
            MSG("  %08X %04X %04X %-20s %08X %08X %08X %08X->%08X %s + %X", rel.r_offset, symEntry, relType, type2String(relType), relAddr, symAddr, sym.st_value, from, to, name, rel.r_addend);
        }
    }
    return r;
#ifdef __linux
err:
    ERR("Error reading relocation data");
    return -1;
#endif
}


/*** Main functions ***/


void elfLoaderFree(ELFLoaderContext_t* ctx) {
    if (ctx) {
        ELFLoaderSection_t* section = ctx->section;
        ELFLoaderSection_t* next;
        while(section != NULL) {
            if (section->data) {
                free(section->data);
            }
            next = section->next;
            free(section);
            section = next;
        }
        free(ctx);
    }
}


ELFLoaderContext_t* elfLoaderInitLoadAndRelocate(LOADER_FD_T fd, const ELFLoaderEnv_t *env) {
    MSG("ENV:");
    for (int i = 0; i < env->exported_size; i++) {
        MSG("  %08X %s", (unsigned int) env->exported[i].ptr, env->exported[i].name);
    }

    ELFLoaderContext_t* ctx = malloc(sizeof(ELFLoaderContext_t));
    assert(ctx);

    memset(ctx, 0, sizeof(ELFLoaderContext_t));
    ctx->fd = fd;
    ctx->env = env;
    {
        Elf32_Ehdr header;
        Elf32_Shdr section;
        /* Load the ELF header, located at the start of the buffer. */
        LOADER_GETDATA(ctx, 0, &header, sizeof(Elf32_Ehdr));

        /* Make sure that we have a correct and compatible ELF header. */
        char ElfMagic[] = { 0x7f, 'E', 'L', 'F', '\0' };
        if (memcmp(header.e_ident, ElfMagic, strlen(ElfMagic)) != 0) {
            ERR("Bad ELF Identification");
            goto err;
        }

        /* Load the section header, get the number of entries of the section header, get a pointer to the actual table of strings */
        LOADER_GETDATA(ctx, header.e_shoff + header.e_shstrndx * sizeof(Elf32_Shdr), &section, sizeof(Elf32_Shdr));
        ctx->e_shnum = header.e_shnum;
        ctx->e_shoff = header.e_shoff;
        ctx->shstrtab_offset = section.sh_offset;
    }

    {
        /* Go through all sections, allocate and copy the relevant ones
        ".symtab": segment contains the symbol table for this file
        ".strtab": segment points to the actual string names used by the symbol table
        */
        MSG("Scanning ELF sections         relAddr      size");
        for (int n = 1; n < ctx->e_shnum; n++) {
            Elf32_Shdr sectHdr;
            char name[33] = "<unamed>";
            if (readSection(ctx, n, &sectHdr, name, sizeof(name)) != 0) {
                ERR("Error reading section");
                goto err;
            }
            if (sectHdr.sh_flags & SHF_ALLOC) {
                if (!sectHdr.sh_size) {
                    MSG("  section %2d: %-15s no data", n, name);
                } else {
                    ELFLoaderSection_t* section = malloc(sizeof(ELFLoaderSection_t));
                    assert(section);
                    memset(section, 0, sizeof(ELFLoaderSection_t));
                    section->next = ctx->section;
                    ctx->section = section;
                    if (sectHdr.sh_flags & SHF_EXECINSTR) {
                        section->data = LOADER_ALLOC_EXEC(sectHdr.sh_size);
                    } else {
                        section->data = LOADER_ALLOC_DATA(sectHdr.sh_size);
                    }
                    if (!section->data) {
                        ERR("Section malloc failled: %s", name);
                        goto err;
                    }
                    section->secIdx = n;
                    section->size = sectHdr.sh_size;
                    if (sectHdr.sh_type != SHT_NOBITS) {
                        LOADER_GETDATA(ctx, sectHdr.sh_offset, section->data, sectHdr.sh_size);
                    }
                    if (strcmp(name, ".text") == 0) {
                        ctx->text = section->data;
                    }
                    MSG("  section %2d: %-15s %08X %6i", n, name, (unsigned int) section->data, sectHdr.sh_size);
                }
            } else if (sectHdr.sh_type == SHT_RELA) {
                if (sectHdr.sh_info >= n) {
                    ERR("Rela section: bad linked section (%i:%s -> %i)", n, name, sectHdr.sh_info);
                    goto err;
                }
                ELFLoaderSection_t* section = findSection(ctx, sectHdr.sh_info);
                if (section == NULL) {
                    MSG("  section %2d: %-15s -> %2d: ignoring", n, name, sectHdr.sh_info);
                } else {
                    section->relSecIdx = n;
                    MSG("  section %2d: %-15s -> %2d: ok", n, name, sectHdr.sh_info);
                }
            } else {
                MSG("  section %2d: %s", n, name);
                if (strcmp(name, ".symtab") == 0) {
                    ctx->symtab_offset = sectHdr.sh_offset;
                    ctx->symtab_count = sectHdr.sh_size / sizeof(Elf32_Sym);
                } else if (strcmp(name, ".strtab") == 0) {
                    ctx->strtab_offset = sectHdr.sh_offset;
                }
            }
        }
        if (ctx->symtab_offset == 0 || ctx->symtab_offset == 0) {
            ERR("Missing .symtab or .strtab section");
            goto err;
        }
    }

    {
        MSG("Relocating sections");
        int r = 0;
        for (ELFLoaderSection_t* section = ctx->section; section != NULL; section = section->next) {
            r |= relocateSection(ctx, section);
        }
        if (r != 0) {
            MSG("Relocation failed");
            goto err;
        }
    }
    return ctx;

err:
    elfLoaderFree(ctx);
    return NULL;
}


int elfLoaderSetFunc(ELFLoaderContext_t *ctx, char* funcname) {
    ctx->exec = 0;
    MSG("Scanning ELF symbols");
    MSG("  Sym  Symbol                         sect value    size relAddr");
    for (int symCount = 0; symCount < ctx->symtab_count; symCount++) {
        Elf32_Sym sym;
        char name[33] = "<unnamed>";
        if (readSymbol(ctx, symCount, &sym, name, sizeof(name)) != 0) {
            ERR("Error reading symbol");
            return -1;
        }
        if(strcmp(name, funcname) == 0) {
            Elf32_Addr symAddr = findSymAddr(ctx, &sym, name);
            if (symAddr == 0xffffffff) {
                MSG("  %04X %-30s %04X %08X %04X ????????", symCount, name, sym.st_shndx, sym.st_value, sym.st_size);
            } else {
                ctx->exec = (void*)symAddr;
                MSG("  %04X %-30s %04X %08X %04X %08X", symCount, name, sym.st_shndx, sym.st_value, sym.st_size, symAddr);
            }
        } else {
            MSG("  %04X %-30s %04X %08X %04X", symCount, name, sym.st_shndx, sym.st_value, sym.st_size);
        }
    }
    if (ctx->exec == 0) {
        ERR("Function symbol not found: %s", funcname);
        return -1;
    }
    return 0;
}


int elfLoaderRun(ELFLoaderContext_t *ctx, int arg) {
    if (!ctx->exec) {
        return 0;
    }
    typedef int (*func_t)(int);
    func_t func = (func_t)ctx->exec;
    MSG("Running...");
    int r = func(arg);
    MSG("Result: %08X", r);
    return r;
}


int elfLoader(LOADER_FD_T fd, const ELFLoaderEnv_t *env, char* funcname, int arg) {
    ELFLoaderContext_t* ctx = elfLoaderInitLoadAndRelocate(fd, env);
    if (!ctx) {
        return -1;
    }
    if (elfLoaderSetFunc(ctx, funcname) != 0) {
        elfLoaderFree(ctx);
        return -1;
    }
    int r = elfLoaderRun(ctx, arg);
    elfLoaderFree(ctx);
    return r;
}

void* elfLoaderGetTextAddr(ELFLoaderContext_t *ctx) {
    return ctx->text;
}
