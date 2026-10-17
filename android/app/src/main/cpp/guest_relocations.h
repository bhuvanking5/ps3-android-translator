#ifndef PS3_GUEST_RELOCATIONS_H
#define PS3_GUEST_RELOCATIONS_H

#include <stddef.h>
#include <stdint.h>
#include "guest_memory.h"

#define PS3_GUEST_RELOCATION_MAX_RECORDS 256u

/* Numeric values follow the ELFv1 PPC64 relocation ABI. This first native
 * implementation intentionally supports only relocations whose result fits
 * the current 32-bit guest-address model. */
typedef enum {
    PS3_GUEST_RELOC_OK = 0,
    PS3_GUEST_RELOC_INVALID_ARGUMENT,
    PS3_GUEST_RELOC_UNSUPPORTED_TYPE,
    PS3_GUEST_RELOC_TARGET_NOT_WRITABLE,
    PS3_GUEST_RELOC_VALUE_OUT_OF_RANGE,
    PS3_GUEST_RELOC_TOO_MANY_RECORDS
} ps3_guest_reloc_status;

enum {
    PS3_PPC64_R_ADDR32 = 1u,
    PS3_PPC64_R_REL32 = 26u
};

typedef struct {
    uint32_t type;
    uint32_t target_address; /* P: guest virtual address of the 4-byte field */
    uint64_t symbol_value;   /* S: already-resolved guest symbol address/value */
    int64_t addend;          /* A: explicit RELA addend */
} ps3_guest_relocation;

/*
 * Apply a bounded batch of big-endian PPC64 RELA relocations to mapped writable
 * guest memory. Supports R_PPC64_ADDR32 (S+A) and R_PPC64_REL32 (S+A-P).
 * All records are validated before any guest bytes change. Symbol lookup,
 * PT_DYNAMIC parsing, load-bias calculation, 64-bit guest memory, and PS3
 * firmware imports are deliberately outside this helper.
 */
ps3_guest_reloc_status ps3_guest_relocations_apply(
    ps3_guest_memory *memory,
    const ps3_guest_relocation *records,
    size_t record_count);

#endif
