#include "guest_elf_loader.h"

#include <limits.h>
#include <string.h>

#define ELF_HEADER_SIZE 64u
#define ELF_PROGRAM_HEADER_SIZE 56u
#define ELF_PT_LOAD 1u
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

ps3_guest_elf_status ps3_guest_elf_load(
    const uint8_t *file, size_t file_size,
    ps3_guest_memory *memory,
    uint8_t *ram, uint32_t ram_base, uint32_t ram_size,
    uint32_t *entry_point) {
    if (file == 0 || memory == 0 || ram == 0 || entry_point == 0 ||
        file_size < ELF_HEADER_SIZE || ram_size == 0u) {
        return PS3_GUEST_ELF_INVALID_ARGUMENT;
    }
    if (file[0] != 0x7fu || file[1] != 'E' || file[2] != 'L' ||
        file[3] != 'F') return PS3_GUEST_ELF_BAD_FORMAT;
    if (file[4] != 2u || file[5] != 2u || file[6] != 1u ||
        be16(file + 18) != ELF_EM_PPC64) return PS3_GUEST_ELF_UNSUPPORTED;
    if (be16(file + 16) != ELF_ET_EXEC || be32(file + 20) != 1u ||
        be16(file + 52) < ELF_HEADER_SIZE) return PS3_GUEST_ELF_UNSUPPORTED;

    const uint64_t entry = be64(file + 24);
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
    const uint64_t ram_end = (uint64_t)ram_base + ram_size;
    if (ram_end > (uint64_t)UINT32_MAX + 1u) {
        return PS3_GUEST_ELF_OUT_OF_BOUNDS;
    }

    for (uint16_t i = 0; i < phnum; ++i) {
        const uint8_t *ph = file + (size_t)(phoff + (uint64_t)i * phentsize);
        if (be32(ph) != ELF_PT_LOAD) continue;
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

    for (size_t i = 0; i < segment_count; ++i) {
        const load_segment *seg = &segments[i];
        const size_t offset = (size_t)(seg->vaddr - ram_base);
        memset(ram + offset, 0, (size_t)seg->memsz);
        memcpy(ram + offset, file + (size_t)seg->offset, (size_t)seg->filesz);
        unsigned permissions = 0u;
        if ((seg->flags & ELF_PF_R) != 0u) permissions |= PS3_GUEST_MEMORY_READ;
        if ((seg->flags & ELF_PF_W) != 0u) permissions |= PS3_GUEST_MEMORY_WRITE;
        if ((seg->flags & ELF_PF_X) != 0u) permissions |= PS3_GUEST_MEMORY_EXECUTE;
        if (!ps3_guest_memory_map(memory, (uint32_t)seg->vaddr,
                (uint32_t)seg->memsz, ram + offset, permissions)) {
            /* Preflight checks make this path defensive, not expected. */
            return PS3_GUEST_ELF_MEMORY_MAP_ERROR;
        }
    }
    *entry_point = (uint32_t)entry;
    return PS3_GUEST_ELF_OK;
}
