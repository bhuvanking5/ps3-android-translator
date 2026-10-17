#ifndef PS3_GUEST_ELF_LOADER_H
#define PS3_GUEST_ELF_LOADER_H

#include <stddef.h>
#include <stdint.h>
#include "guest_memory.h"

typedef enum {
    PS3_GUEST_ELF_OK = 0,
    PS3_GUEST_ELF_INVALID_ARGUMENT,
    PS3_GUEST_ELF_BAD_FORMAT,
    PS3_GUEST_ELF_UNSUPPORTED,
    PS3_GUEST_ELF_OUT_OF_BOUNDS,
    PS3_GUEST_ELF_MEMORY_MAP_ERROR,
    PS3_GUEST_ELF_RELOCATION_REQUIRED
} ps3_guest_elf_status;

/*
 * Load an unencrypted, big-endian ELF64 PowerPC64 executable into caller-owned
 * guest RAM. This maps PT_LOAD segments only; it does not handle PS3 SELF
 * decryption, relocations, imports, firmware, or process/OS setup.
 * Addresses above 32 bits are rejected by this prototype memory model. ET_DYN
 * files and files containing PT_DYNAMIC are rejected until native relocation
 * and import resolution are implemented; use RELOCATION_REQUIRED to distinguish
 * that limitation from malformed input.
 */
ps3_guest_elf_status ps3_guest_elf_load(
    const uint8_t *file, size_t file_size,
    ps3_guest_memory *memory,
    uint8_t *ram, uint32_t ram_base, uint32_t ram_size,
    uint32_t *entry_point);

#endif
