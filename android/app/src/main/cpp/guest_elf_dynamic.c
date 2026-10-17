#include "guest_elf_dynamic.h"

#include <limits.h>
#include <string.h>
#include <stdio.h>

#define ELF_HEADER_SIZE 64u
#define ELF_PROGRAM_HEADER_SIZE 56u
#define ELF64_SYMBOL_SIZE 24u
#define ELF64_RELA_SIZE 24u
#define ELF64_DYNAMIC_SIZE 16u
#define ELF_ET_EXEC 2u
#define ELF_EM_PPC64 21u
#define ELF_PT_LOAD 1u
#define ELF_PT_DYNAMIC 2u
#define ELF_PF_W 2u
#define ELF_SHN_UNDEF 0u
#define DT_NULL 0
#define DT_STRTAB 5
#define DT_SYMTAB 6
#define DT_RELA 7
#define DT_RELASZ 8
#define DT_RELAENT 9
#define DT_STRSZ 10
#define DT_SYMENT 11
#define MAX_PROGRAM_HEADERS 128u
#define MAX_DYNAMIC_ENTRIES 4096u
#define MAX_SYMBOL_INDEX 4096u

typedef struct {
    uint64_t offset;
    uint64_t address;
    uint64_t file_size;
    uint64_t memory_size;
    uint32_t flags;
} load_segment;

static uint16_t read_be16(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}
static uint32_t read_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}
static uint64_t read_be64(const uint8_t *p) {
    return ((uint64_t)read_be32(p) << 32) | read_be32(p + 4);
}
static int file_range_ok(size_t file_size, uint64_t offset, uint64_t length) {
    return offset <= (uint64_t)file_size &&
           length <= (uint64_t)file_size - offset;
}
static int virtual_to_file(const load_segment *segments, size_t count,
                           uint64_t address, uint64_t length, uint64_t *offset) {
    for (size_t i = 0; i < count; ++i) {
        const load_segment *s = &segments[i];
        if (address >= s->address &&
            address - s->address <= s->file_size &&
            length <= s->file_size - (address - s->address)) {
            if (s->offset > UINT64_MAX - (address - s->address)) return 0;
            *offset = s->offset + (address - s->address);
            return 1;
        }
    }
    return 0;
}

ps3_guest_dynamic_status ps3_guest_elf_parse_rela(
    const uint8_t *file, size_t file_size,
    ps3_guest_relocation *records, size_t record_capacity,
    size_t *record_count, ps3_guest_symbol_resolver resolver,
    void *resolver_context) {
    if (file == NULL || records == NULL || record_count == NULL ||
        file_size < ELF_HEADER_SIZE ||
        record_capacity > PS3_GUEST_ELF_DYNAMIC_MAX_RELOCS) {
        return PS3_GUEST_DYNAMIC_INVALID_ARGUMENT;
    }
    *record_count = 0u;
    if (memcmp(file, "\x7f" "ELF", 4u) != 0 || file[4] != 2u ||
        file[5] != 2u || file[6] != 1u ||
        read_be16(file + 18u) != ELF_EM_PPC64 ||
        read_be32(file + 20u) != 1u) {
        return PS3_GUEST_DYNAMIC_UNSUPPORTED;
    }
    if (read_be16(file + 16u) != ELF_ET_EXEC) {
        return PS3_GUEST_DYNAMIC_UNSUPPORTED;
    }

    fprintf(stderr, "DBG header ok\\n");
    const uint64_t phoff = read_be64(file + 32u);
    const uint16_t phentsize = read_be16(file + 54u);
    const uint16_t phnum = read_be16(file + 56u);
    if (phentsize < ELF_PROGRAM_HEADER_SIZE || phnum > MAX_PROGRAM_HEADERS ||
        phoff > file_size ||
        (uint64_t)phnum > ((uint64_t)file_size - phoff) / phentsize) {
        return PS3_GUEST_DYNAMIC_BAD_FORMAT;
    }

    load_segment segments[MAX_PROGRAM_HEADERS];
    size_t segment_count = 0u;
    uint64_t dynamic_offset = 0u, dynamic_address = 0u, dynamic_size = 0u;
    int have_dynamic = 0;
    for (uint16_t i = 0; i < phnum; ++i) {
        const uint8_t *ph = file + (size_t)(phoff + (uint64_t)i * phentsize);
        const uint32_t type = read_be32(ph);
        if (type == ELF_PT_LOAD) {
            if (segment_count >= MAX_PROGRAM_HEADERS) {
                return PS3_GUEST_DYNAMIC_BAD_FORMAT;
            }
            load_segment *s = &segments[segment_count++];
            s->flags = read_be32(ph + 4u);
            s->offset = read_be64(ph + 8u);
            s->address = read_be64(ph + 16u);
            s->file_size = read_be64(ph + 32u);
            s->memory_size = read_be64(ph + 40u);
            if (s->file_size > s->memory_size ||
                !file_range_ok(file_size, s->offset, s->file_size) ||
                s->address > UINT64_MAX - s->memory_size) {
                return PS3_GUEST_DYNAMIC_BAD_FORMAT;
            }
            for (size_t j = 0; j + 1u < segment_count; ++j) {
                const load_segment *previous = &segments[j];
                if (s->address < previous->address + previous->memory_size &&
                    previous->address < s->address + s->memory_size) {
                    return PS3_GUEST_DYNAMIC_BAD_FORMAT;
                }
            }
        } else if (type == ELF_PT_DYNAMIC) {
            if (have_dynamic) return PS3_GUEST_DYNAMIC_BAD_FORMAT;
            have_dynamic = 1;
            dynamic_offset = read_be64(ph + 8u);
            dynamic_address = read_be64(ph + 16u);
            dynamic_size = read_be64(ph + 32u);
            if (dynamic_size == 0u || dynamic_size % ELF64_DYNAMIC_SIZE != 0u ||
                !file_range_ok(file_size, dynamic_offset, dynamic_size) ||
                dynamic_size / ELF64_DYNAMIC_SIZE > MAX_DYNAMIC_ENTRIES) {
                return PS3_GUEST_DYNAMIC_BAD_FORMAT;
            }
        }
    }
    if (!have_dynamic) return PS3_GUEST_DYNAMIC_BAD_FORMAT;

    uint64_t mapped_dynamic_offset = 0u;
    if (!virtual_to_file(segments, segment_count, dynamic_address,
                         dynamic_size, &mapped_dynamic_offset) ||
        mapped_dynamic_offset != dynamic_offset) {
        return PS3_GUEST_DYNAMIC_BAD_FORMAT;
    }

    uint64_t rela_address = 0u, rela_size = 0u, rela_entry_size = 0u;
    uint64_t symbol_table_address = 0u, symbol_entry_size = 0u;
    uint64_t string_table_address = 0u, string_table_size = 0u;
    int have_rela = 0, have_rela_size = 0, have_rela_entry = 0;
    int have_symtab = 0, have_syment = 0, have_strtab = 0, have_strsz = 0;
    int terminated = 0;
    for (uint64_t offset = 0u; offset < dynamic_size;
         offset += ELF64_DYNAMIC_SIZE) {
        const uint8_t *entry = file + (size_t)(dynamic_offset + offset);
        const int64_t tag = (int64_t)read_be64(entry);
        const uint64_t value = read_be64(entry + 8u);
        if (tag == DT_NULL) {
            terminated = 1;
            break;
        }
#define TAKE_TAG(tag_value, target, seen) \
        if (tag == (tag_value)) { if (seen) return PS3_GUEST_DYNAMIC_BAD_FORMAT; \
            (target) = value; (seen) = 1; }
        TAKE_TAG(DT_RELA, rela_address, have_rela)
        else TAKE_TAG(DT_RELASZ, rela_size, have_rela_size)
        else TAKE_TAG(DT_RELAENT, rela_entry_size, have_rela_entry)
        else TAKE_TAG(DT_SYMTAB, symbol_table_address, have_symtab)
        else TAKE_TAG(DT_SYMENT, symbol_entry_size, have_syment)
        else TAKE_TAG(DT_STRTAB, string_table_address, have_strtab)
        else TAKE_TAG(DT_STRSZ, string_table_size, have_strsz)
#undef TAKE_TAG
    }
    if (!terminated) return PS3_GUEST_DYNAMIC_BAD_FORMAT;
    if (!have_rela && !have_rela_size && !have_rela_entry) return PS3_GUEST_DYNAMIC_OK;
    if (!have_rela || !have_rela_size || !have_rela_entry ||
        rela_entry_size != ELF64_RELA_SIZE || rela_size % ELF64_RELA_SIZE != 0u) {
        return PS3_GUEST_DYNAMIC_BAD_FORMAT;
    }
    if ((have_symtab != have_syment) ||
        (have_strtab != have_strsz)) {
        return PS3_GUEST_DYNAMIC_BAD_FORMAT;
    }
    if (have_strtab) {
        uint64_t string_offset = 0u;
        if (!virtual_to_file(segments, segment_count, string_table_address,
                             string_table_size, &string_offset)) {
            return PS3_GUEST_DYNAMIC_BAD_FORMAT;
        }
    }

    fprintf(stderr, "DBG dyn rela=%llx size=%llu symtab=%llx syment=%llu strtab=%llx strsz=%llu\\n", (unsigned long long)rela_address, (unsigned long long)rela_size, (unsigned long long)symbol_table_address, (unsigned long long)symbol_entry_size, (unsigned long long)string_table_address, (unsigned long long)string_table_size);
    const uint64_t relocation_count64 = rela_size / ELF64_RELA_SIZE;
    if (relocation_count64 > record_capacity ||
        relocation_count64 > PS3_GUEST_ELF_DYNAMIC_MAX_RELOCS) {
        return PS3_GUEST_DYNAMIC_TOO_MANY_RELOCS;
    }
    uint64_t rela_offset = 0u;
    if (rela_size != 0u &&
        !virtual_to_file(segments, segment_count, rela_address, rela_size, &rela_offset)) {
        return PS3_GUEST_DYNAMIC_OUT_OF_BOUNDS;
    }

    ps3_guest_relocation staged[PS3_GUEST_ELF_DYNAMIC_MAX_RELOCS];
    const size_t relocation_count = (size_t)relocation_count64;
    for (size_t i = 0; i < relocation_count; ++i) {
        const uint8_t *rela = file + (size_t)(rela_offset + i * ELF64_RELA_SIZE);
        const uint64_t target = read_be64(rela);
        const uint64_t info = read_be64(rela + 8u);
        const uint64_t raw_addend = read_be64(rela + 16u);
        const uint32_t relocation_type = (uint32_t)info;
        const uint32_t symbol_index = (uint32_t)(info >> 32);
        int target_is_writable = 0;
        uint64_t symbol_value = 0u;

        fprintf(stderr, "DBG reloc %zu type=%u sym=%u target=%llx\\n", i, relocation_type, symbol_index, (unsigned long long)target);
        if (relocation_type != PS3_PPC64_R_ADDR32 &&
            relocation_type != PS3_PPC64_R_REL32) {
            return PS3_GUEST_DYNAMIC_UNSUPPORTED;
        }
        if (target > UINT32_MAX - 3u || (target & 3u) != 0u) {
            return PS3_GUEST_DYNAMIC_OUT_OF_BOUNDS;
        }
        for (size_t j = 0; j < segment_count; ++j) {
            const load_segment *s = &segments[j];
            if ((s->flags & ELF_PF_W) != 0u && target >= s->address &&
                target - s->address <= s->memory_size &&
                4u <= s->memory_size - (target - s->address)) {
                target_is_writable = 1;
                break;
            }
        }
        if (!target_is_writable) return PS3_GUEST_DYNAMIC_OUT_OF_BOUNDS;

        if (symbol_index != 0u) {
            uint64_t symbol_offset = 0u;
            if (!have_symtab || !have_syment ||
                symbol_entry_size != ELF64_SYMBOL_SIZE ||
                symbol_index > MAX_SYMBOL_INDEX ||
                symbol_index > (UINT64_MAX - symbol_table_address) / ELF64_SYMBOL_SIZE ||
                !virtual_to_file(segments, segment_count,
                    symbol_table_address + (uint64_t)symbol_index * ELF64_SYMBOL_SIZE,
                    ELF64_SYMBOL_SIZE, &symbol_offset)) {
                return PS3_GUEST_DYNAMIC_UNSUPPORTED;
            }
            const uint8_t *symbol = file + (size_t)symbol_offset;
            fprintf(stderr, "DBG symtab=%llx idx=%u off=%llu shndx=%u value=%llx name=%u\\n", (unsigned long long)symbol_table_address, symbol_index, (unsigned long long)symbol_offset, read_be16(symbol + 6u), (unsigned long long)read_be64(symbol + 8u), read_be32(symbol));
            const uint16_t section_index = read_be16(symbol + 6u);
            const uint32_t name_offset = read_be32(symbol);
            symbol_value = read_be64(symbol + 8u);
            const char *symbol_name = NULL;
            if (have_strtab) {
                uint64_t string_offset = 0u;
                if (name_offset >= string_table_size ||
                    !virtual_to_file(segments, segment_count, string_table_address,
                                     string_table_size, &string_offset) ||
                    memchr(file + (size_t)string_offset + name_offset, '\0',
                           (size_t)(string_table_size - name_offset)) == NULL) {
                    return PS3_GUEST_DYNAMIC_BAD_FORMAT;
                }
                symbol_name = (const char *)(file + (size_t)string_offset + name_offset);
            }
            if (section_index == ELF_SHN_UNDEF) {
                if (resolver == NULL || symbol_name == NULL) {
                    return PS3_GUEST_DYNAMIC_UNSUPPORTED;
                }
                if (!resolver(symbol_name, &symbol_value, resolver_context) ||
                    symbol_value > UINT32_MAX) {
                    return PS3_GUEST_DYNAMIC_UNSUPPORTED;
                }
            } else if (symbol_value > UINT32_MAX) {
                return PS3_GUEST_DYNAMIC_UNSUPPORTED;
            }
        }

        staged[i].type = relocation_type;
        staged[i].target_address = (uint32_t)target;
        staged[i].symbol_value = symbol_value;
        staged[i].addend = raw_addend <= (uint64_t)INT64_MAX
            ? (int64_t)raw_addend
            : -1 - (int64_t)(UINT64_MAX - raw_addend);
    }
    if (relocation_count != 0u) {
        memcpy(records, staged, relocation_count * sizeof(staged[0]));
    }
    *record_count = relocation_count;
    return PS3_GUEST_DYNAMIC_OK;
}
