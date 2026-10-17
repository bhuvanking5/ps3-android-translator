#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "guest_relocations.h"

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "CHECK failed at line %d: %s\n", __LINE__, #c); return 1; } } while (0)

int main(void) {
    ps3_guest_memory memory;
    uint8_t ram[32];
    memset(ram, 0xa5, sizeof(ram));
    ps3_guest_memory_init(&memory);
    CHECK(ps3_guest_memory_map(&memory, 0x1000u, sizeof(ram), ram,
        PS3_GUEST_MEMORY_READ | PS3_GUEST_MEMORY_WRITE));

    ps3_guest_relocation records[2] = {
        { PS3_PPC64_R_ADDR32, 0x1000u, 0x12340000u, 0x24 },
        { PS3_PPC64_R_REL32, 0x1004u, 0x1010u, 4 }
    };
    CHECK(ps3_guest_relocations_apply(&memory, records, 2u) == PS3_GUEST_RELOC_OK);
    CHECK(ram[0] == 0x12u && ram[1] == 0x34u && ram[2] == 0u && ram[3] == 0x24u);
    CHECK(ram[4] == 0u && ram[5] == 0u && ram[6] == 0u && ram[7] == 0x10u);

    /* A later invalid record must not leave the earlier target modified. */
    memset(ram, 0xa5, sizeof(ram));
    records[0] = (ps3_guest_relocation){ PS3_PPC64_R_ADDR32, 0x1000u, 7u, 0 };
    records[1] = (ps3_guest_relocation){ 999u, 0x1004u, 9u, 0 };
    CHECK(ps3_guest_relocations_apply(&memory, records, 2u) == PS3_GUEST_RELOC_UNSUPPORTED_TYPE);
    CHECK(ram[0] == 0xa5u && ram[3] == 0xa5u && ram[4] == 0xa5u);

    records[0] = (ps3_guest_relocation){ PS3_PPC64_R_ADDR32, 0x1000u, UINT64_MAX, 1 };
    CHECK(ps3_guest_relocations_apply(&memory, records, 1u) == PS3_GUEST_RELOC_VALUE_OUT_OF_RANGE);
    CHECK(ram[0] == 0xa5u && ram[3] == 0xa5u);

    records[0] = (ps3_guest_relocation){ PS3_PPC64_R_ADDR32, 0x2000u, 1u, 0 };
    CHECK(ps3_guest_relocations_apply(&memory, records, 1u) == PS3_GUEST_RELOC_TARGET_NOT_WRITABLE);

    uint8_t read_only[4] = {0};
    CHECK(ps3_guest_memory_map(&memory, 0x2000u, sizeof(read_only), read_only,
        PS3_GUEST_MEMORY_READ));
    records[0] = (ps3_guest_relocation){ PS3_PPC64_R_ADDR32, 0x2000u, 1u, 0 };
    CHECK(ps3_guest_relocations_apply(&memory, records, 1u) == PS3_GUEST_RELOC_TARGET_NOT_WRITABLE);

    records[0] = (ps3_guest_relocation){ PS3_PPC64_R_REL32, 0x1000u, 0u, 0 };
    CHECK(ps3_guest_relocations_apply(&memory, records, 1u) == PS3_GUEST_RELOC_OK);
    CHECK(ram[0] == 0xffu && ram[1] == 0xffu && ram[2] == 0xf0u && ram[3] == 0u);

    CHECK(ps3_guest_relocations_apply(&memory, 0, 0u) == PS3_GUEST_RELOC_OK);
    CHECK(ps3_guest_relocations_apply(0, records, 1u) == PS3_GUEST_RELOC_INVALID_ARGUMENT);
    puts("Native guest relocation tests passed.");
    return 0;
}
