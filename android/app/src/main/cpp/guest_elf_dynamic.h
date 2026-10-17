#ifndef PS3_GUEST_ELF_DYNAMIC_H
#define PS3_GUEST_ELF_DYNAMIC_H
#include <stddef.h>
#include <stdint.h>
#include "guest_relocations.h"
#define PS3_GUEST_ELF_DYNAMIC_MAX_RELOCS PS3_GUEST_RELOCATION_MAX_RECORDS
typedef enum { PS3_GUEST_DYNAMIC_OK=0, PS3_GUEST_DYNAMIC_INVALID_ARGUMENT, PS3_GUEST_DYNAMIC_BAD_FORMAT, PS3_GUEST_DYNAMIC_UNSUPPORTED, PS3_GUEST_DYNAMIC_OUT_OF_BOUNDS, PS3_GUEST_DYNAMIC_TOO_MANY_RELOCS } ps3_guest_dynamic_status;
/* Restricted parser: ELF64 BE PPC64 ET_EXEC, symbol-zero ADDR32/REL32 only. */
ps3_guest_dynamic_status ps3_guest_elf_parse_rela(const uint8_t *file,size_t file_size,ps3_guest_relocation *records,size_t record_capacity,size_t *record_count);
#endif
