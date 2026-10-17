#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "guest_elf_dynamic.h"
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "CHECK failed line %d: %s\n", __LINE__, #c); return 1; } } while (0)
static void p16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }
static void p32(uint8_t *p, uint32_t v) { p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v; }
static void p64(uint8_t *p, uint64_t v) { p32(p, (uint32_t)(v >> 32)); p32(p + 4, (uint32_t)v); }
static int resolve_foo(const char *name, uint64_t *value, void *context) {
    int *calls = (int *)context;
    ++*calls;
    if (strcmp(name, "foo") != 0) return 0;
    *value = 0x1020u;
    return 1;
}
int main(void) {
    uint8_t f[448] = {0};
    memcpy(f, "\\x7f" "ELF", 4u);
    f[4] = 2; f[5] = 2; f[6] = 1;
    p16(f + 16, 2); p16(f + 18, 21); p32(f + 20, 1);
    p64(f + 32, 64); p16(f + 52, 64); p16(f + 54, 56); p16(f + 56, 2);
    p32(f + 64, 1); p32(f + 68, 6); p64(f + 72, 176); p64(f + 80, 0x1000);
    p64(f + 96, 272); p64(f + 104, 272); p64(f + 112, 8);
    p32(f + 120, 2); p64(f + 128, 176); p64(f + 136, 0x1000);
    p64(f + 152, 128); p64(f + 160, 128); p64(f + 168, 8);
    p64(f + 176, 7); p64(f + 184, 0x1090);
    p64(f + 192, 8); p64(f + 200, 48);
    p64(f + 208, 9); p64(f + 216, 24);
    p64(f + 224, 6); p64(f + 232, 0x10c0);
    p64(f + 240, 11); p64(f + 248, 24);
    p64(f + 256, 5); p64(f + 264, 0x10f0);
    p64(f + 272, 10); p64(f + 280, 16);
    p64(f + 288, 0); p64(f + 296, 0);
    p64(f + 320, 0x1080); p64(f + 328, (1ull << 32) | 1u); p64(f + 336, 0x20);
    p64(f + 344, 0x1084); p64(f + 352, 26); p64(f + 360, 0xfffffffffffffff0ull);
    p32(f + 392, 1); p16(f + 398, 1); p64(f + 400, 0x1010);
    f[416] = 0; memcpy(f + 417, "foo", 4u);

    ps3_guest_relocation out[4];
    size_t n = 99;
    ps3_guest_dynamic_status initial_status = ps3_guest_elf_parse_rela(f, sizeof(f), out, 4, &n, 0, 0);\n    if (initial_status != PS3_GUEST_DYNAMIC_OK) { fprintf(stderr, "initial parser status=%d\\n", (int)initial_status); return 1; }
    CHECK(n == 2);
    CHECK(out[0].type == PS3_PPC64_R_ADDR32 && out[0].target_address == 0x1080 &&
          out[0].symbol_value == 0x1010 && out[0].addend == 0x20);
    CHECK(out[1].type == PS3_PPC64_R_REL32 && out[1].target_address == 0x1084 &&
          out[1].symbol_value == 0u && out[1].addend == -16);
    CHECK(ps3_guest_elf_parse_rela(f, sizeof(f), out, 1, &n, 0, 0) == PS3_GUEST_DYNAMIC_TOO_MANY_RELOCS);

    int resolver_calls = 0;
    p16(f + 398, 0);
    CHECK(ps3_guest_elf_parse_rela(f, sizeof(f), out, 4, &n, 0, 0) == PS3_GUEST_DYNAMIC_UNSUPPORTED);
    CHECK(ps3_guest_elf_parse_rela(f, sizeof(f), out, 4, &n, resolve_foo, &resolver_calls) == PS3_GUEST_DYNAMIC_OK);
    CHECK(resolver_calls == 1 && out[0].symbol_value == 0x1020u);
    resolver_calls = 0;
    CHECK(ps3_guest_elf_parse_rela(f, sizeof(f), out, 4, &n, resolve_foo, &resolver_calls) == PS3_GUEST_DYNAMIC_OK);
    CHECK(resolver_calls == 1);
    p16(f + 398, 1);

    p64(f + 352, 999u);
    CHECK(ps3_guest_elf_parse_rela(f, sizeof(f), out, 4, &n, 0, 0) == PS3_GUEST_DYNAMIC_UNSUPPORTED);
    p64(f + 352, 26);
    p64(f + 320, 0x2000);
    CHECK(ps3_guest_elf_parse_rela(f, sizeof(f), out, 4, &n, 0, 0) == PS3_GUEST_DYNAMIC_OUT_OF_BOUNDS);
    p64(f + 320, 0x1080);
    p64(f + 288, 1);
    CHECK(ps3_guest_elf_parse_rela(f, sizeof(f), out, 4, &n, 0, 0) == PS3_GUEST_DYNAMIC_BAD_FORMAT);
    p64(f + 288, 0);
    p64(f + 136, 0x2000);
    CHECK(ps3_guest_elf_parse_rela(f, sizeof(f), out, 4, &n, 0, 0) == PS3_GUEST_DYNAMIC_BAD_FORMAT);
    CHECK(ps3_guest_elf_parse_rela(0, sizeof(f), out, 4, &n, 0, 0) == PS3_GUEST_DYNAMIC_INVALID_ARGUMENT);
    puts("Native dynamic RELA parser tests passed.");
    return 0;
}
