#include "guest_elf_loader.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "guest_elf_dynamic.h"
#include "guest_relocations.h"

#define ELF_HEADER_SIZE 64u
#define ELF_PROGRAM_HEADER_SIZE 56u
#define ELF_PT_LOAD 1u
#define ELF_PT_DYNAMIC 2u
#define ELF_ET_EXEC 2u
#define ELF_EM_PPC64 21u
#define ELF_PF_X 1u
#define ELF_PF_W 2u
#define ELF_PF_R 4u

typedef struct {
    uint64_t offset;
    uint64_t vaddr;
    uint64_t filesz;
    uint64_t memsz;
    uint32_t flags;
} load_segment;

static uint16_t be16(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}
static uint32_t be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}
static uint64_t be64(const uint8_t *p) {
    return ((uint64_t)be32(p) << 32) | be32(p + 4);
}

ps3_guest_elf_status ps3_guest_elf_load_with_resolver(
    const uint8_t *file, size_t file_size,
    ps3_guest_memory *memory,
    uint8_t *ram, uint32_t ram_base, uint32_t ram_size,
    uint32_t *entry_point,
    ps3_guest_symbol_resolver resolver, void *resolver_context) {
    if (file == 0 || memory == 0 || ram == 0 || entry_point == 0 ||
        file_size < ELF_HEADER_SIZE || ram_size == 0u) {
        return PS3_GUEST_ELF_INVALID_ARGUMENT;
    }
    if (file[0] != 0x7fu || file[1] != 'E' || file[2] != 'L' ||
        file[3] != 'F') return PS3_GUEST_ELF_BAD_FORMAT;
    if (file[4] != 2u || file[5] != 2u || file[6] != 1u ||
        be16(file + 18) != ELF_EM_PPC64) return PS3_GUEST_ELF_UNSUPPORTED;
    const uint16_t elf_type = be16(file + 16);
    if (be32(file + 20) != 1u || be16(file + 52) < ELF_HEADER_SIZE)
        return PS3_GUEST_ELF_UNSUPPORTED;
    if (elf_type == 3u) return PS3_GUEST_ELF_RELOCATION_REQUIRED;
    if (elf_type != ELF_ET_EXEC) return PS3_GUEST_ELF_UNSUPPORTED;

    const uint64_t entry = be64(file + 24);
    /* PowerPC instructions are 4-byte aligned; reject malformed entry PCs. */
    if ((entry & 3u) != 0u) return PS3_GUEST_ELF_BAD_FORMAT;
    const uint64_t phoff = be64(file + 32);
    const uint16_t phentsize = be16(file + 54);
    const uint16_t phnum = be16(file + 56);
    if (phentsize < ELF_PROGRAM_HEADER_SIZE ||
        phoff > file_size ||
        (uint64_t)phnum > ((uint64_t)file_size - phoff) / phentsize) {
        return PS3_GUEST_ELF_BAD_FORMAT;
    }

    load_segment segments[PS3_GUEST_MEMORY_MAX_REGIONS];
    size_t segment_count = 0u;
    int entry_is_executable = 0;
    int has_dynamic = 0;
    const uint64_t ram_end = (uint64_t)ram_base + ram_size;
    if (ram_end > (uint64_t)UINT32_MAX + 1u) {
        return PS3_GUEST_ELF_OUT_OF_BOUNDS;
    }

    for (uint16_t i = 0; i < phnum; ++i) {
        const uint8_t *ph = file + (size_t)(phoff + (uint64_t)i * phentsize);
        const uint32_t program_type = be32(ph);
        if (program_type == ELF_PT_DYNAMIC) {
            if (has_dynamic) return PS3_GUEST_ELF_RELOCATION_REQUIRED;
            has_dynamic = 1;
            continue;
        }
        if (program_type != ELF_PT_LOAD) continue;
        if (segment_count >= PS3_GUEST_MEMORY_MAX_REGIONS) {
            return PS3_GUEST_ELF_UNSUPPORTED;
        }
        load_segment seg;
        seg.flags = be32(ph + 4);
        seg.offset = be64(ph + 8);
        seg.vaddr = be64(ph + 16);
        seg.filesz = be64(ph + 32);
        seg.memsz = be64(ph + 40);
        if (seg.filesz > seg.memsz || seg.offset > file_size ||
            seg.filesz > (uint64_t)file_size - seg.offset) {
            return PS3_GUEST_ELF_BAD_FORMAT;
        }
        if (seg.memsz == 0u) continue;
        if (seg.vaddr > UINT32_MAX || seg.memsz > UINT32_MAX ||
            seg.vaddr < ram_base || seg.vaddr + seg.memsz < seg.vaddr ||
            seg.vaddr + seg.memsz > ram_end ||
            (seg.flags & ~(ELF_PF_R | ELF_PF_W | ELF_PF_X)) != 0u ||
            (seg.flags & (ELF_PF_R | ELF_PF_W | ELF_PF_X)) == 0u) {
            return PS3_GUEST_ELF_OUT_OF_BOUNDS;
        }
        for (size_t j = 0; j < segment_count; ++j) {
            const uint64_t a0 = seg.vaddr;
            const uint64_t a1 = seg.vaddr + seg.memsz;
            const uint64_t b0 = segments[j].vaddr;
            const uint64_t b1 = b0 + segments[j].memsz;
            if (a0 < b1 && b0 < a1) return PS3_GUEST_ELF_BAD_FORMAT;
        }
        if ((seg.flags & ELF_PF_X) != 0u &&
            entry >= seg.vaddr && entry < seg.vaddr + seg.memsz) {
            entry_is_executable = 1;
        }
        segments[segment_count++] = seg;
    }

    if (segment_count == 0u || !entry_is_executable ||
        memory->count + segment_count > PS3_GUEST_MEMORY_MAX_REGIONS) {
        return PS3_GUEST_ELF_BAD_FORMAT;
    }

    /* Validate map collisions before changing guest RAM or the map. */
    for (size_t i = 0; i < segment_count; ++i) {
        const uint64_t start = segments[i].vaddr;
        const uint64_t end = start + segments[i].memsz;
        for (size_t j = 0; j < memory->count; ++j) {
            const ps3_guest_memory_region *r = &memory->regions[j];
            if (start < (uint64_t)r->base + r->size &&
                (uint64_t)r->base < end) return PS3_GUEST_ELF_MEMORY_MAP_ERROR;
        }
    }

    ps3_guest_relocation relocations[PS3_GUEST_RELOCATION_MAX_RECORDS];
    size_t relocation_count = 0u;
    if (has_dynamic) {
        const ps3_guest_dynamic_status dynamic_status =
            ps3_guest_elf_parse_rela(file, file_size, relocations,
                PS3_GUEST_RELOCATION_MAX_RECORDS, &relocation_count,
                resolver, resolver_context);
        if (dynamic_status != PS3_GUEST_DYNAMIC_OK) {
            return PS3_GUEST_ELF_RELOCATION_REQUIRED;
        }
    }

    /* Stage the entire RAM image and map before any externally visible mutation. */
    uint8_t *staged_ram = (uint8_t *)malloc((size_t)ram_size);
    if (staged_ram == 0) return PS3_GUEST_ELF_ALLOCATION_FAILED;
    memcpy(staged_ram, ram, (size_t)ram_size);
    ps3_guest_memory staged_memory = *memory;

    for (size_t i = 0; i < segment_count; ++i) {
        const load_segment *seg = &segments[i];
        const size_t offset = (size_t)(seg->vaddr - ram_base);
        memset(staged_ram + offset, 0, (size_t)seg->memsz);
        memcpy(staged_ram + offset, file + (size_t)seg->offset,
               (size_t)seg->filesz);
        unsigned permissions = 0u;
        if ((seg->flags & ELF_PF_R) != 0u) permissions |= PS3_GUEST_MEMORY_READ;
        if ((seg->flags & ELF_PF_W) != 0u) permissions |= PS3_GUEST_MEMORY_WRITE;
        if ((seg->flags & ELF_PF_X) != 0u) permissions |= PS3_GUEST_MEMORY_EXECUTE;
        if (!ps3_guest_memory_map(&staged_memory, (uint32_t)seg->vaddr,
                (uint32_t)seg->memsz, staged_ram + offset, permissions)) {
            free(staged_ram);
            return PS3_GUEST_ELF_MEMORY_MAP_ERROR;
        }
    }

    if (has_dynamic && ps3_guest_relocations_apply(&staged_memory,
            relocations, relocation_count) != PS3_GUEST_RELOC_OK) {
        free(staged_ram);
        return PS3_GUEST_ELF_RELOCATION_REQUIRED;
    }

    memcpy(ram, staged_ram, (size_t)ram_size);
    *memory = staged_memory;
    /* New regions must point at caller-owned RAM, not the temporary staging copy. */
    for (size_t i = memory->count - segment_count; i < memory->count; ++i) {
        const uint32_t offset = memory->regions[i].base - ram_base;
        memory->regions[i].bytes = ram + offset;
    }
    free(staged_ram);
    *entry_point = (uint32_t)entry;
    return PS3_GUEST_ELF_OK;
}

ps3_guest_elf_status ps3_guest_elf_load(
    const uint8_t *file, size_t file_size,
    ps3_guest_memory *memory,
    uint8_t *ram, uint32_t ram_base, uint32_t ram_size,
    uint32_t *entry_point) {
    return ps3_guest_elf_load_with_resolver(file, file_size, memory, ram,
        ram_base, ram_size, entry_point, NULL, NULL);
}
