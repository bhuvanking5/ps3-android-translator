#include "ppc_to_arm64.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static uint32_t addi(unsigned rt, unsigned ra, int imm) {
    return (14u << 26) | (rt << 21) | (ra << 16) |
           ((uint32_t)imm & 0xffffu);
}

static uint32_t xo(unsigned rt, unsigned ra, unsigned rb, unsigned ext) {
    return (31u << 26) | (rt << 21) | (ra << 16) |
           (rb << 11) | (ext << 1);
}

int main(void) {
    uint32_t output[PPC_TO_ARM64_MAX_WORDS] = {0};
    size_t output_count = 0;
    const uint32_t program[] = {
        addi(3, 0, 7),
        addi(4, 3, 5),
        xo(5, 3, 4, 266),
        xo(6, 3, 5, 40)
    };
    const uint32_t expected[] = {
        0xD28000E3u, /* movz x3, #7 */
        0x91001464u, /* add x4, x3, #5 */
        0x8B040065u, /* add x5, x3, x4 */
        0xCB0300A6u, /* sub x6, x5, x3 */
        0xAA0603E0u, /* mov x0, x6 */
        0xD65F03C0u  /* ret */
    };

    CHECK(ppc_translate_to_arm64(program, 4, 6, output,
          PPC_TO_ARM64_MAX_WORDS, &output_count) == PPC_TO_ARM64_OK);
    CHECK(output_count == sizeof(expected) / sizeof(expected[0]));
    CHECK(memcmp(output, expected, sizeof(expected)) == 0);

    const uint32_t negative[] = { addi(3, 0, -2) };
    CHECK(ppc_translate_to_arm64(negative, 1, 3, output,
          PPC_TO_ARM64_MAX_WORDS, &output_count) == PPC_TO_ARM64_OK);
    CHECK(output[0] == 0x92800023u); /* movn x3, #1 => -2 */

    const uint32_t unsupported[] = { 0x40000004u }; /* conditional branch */
    CHECK(ppc_translate_to_arm64(unsupported, 1, 3, output,
          PPC_TO_ARM64_MAX_WORDS, &output_count) ==
          PPC_TO_ARM64_UNSUPPORTED_INSTRUCTION);
    CHECK(output_count == 0);

    const uint32_t reserved_register[] = { addi(19, 0, 1) };
    CHECK(ppc_translate_to_arm64(reserved_register, 1, 3, output,
          PPC_TO_ARM64_MAX_WORDS, &output_count) ==
          PPC_TO_ARM64_UNSUPPORTED_INSTRUCTION);

    CHECK(ppc_translate_to_arm64(program, 4, 6, output, 5,
          &output_count) == PPC_TO_ARM64_OUTPUT_TOO_SMALL);
    CHECK(ppc_translate_to_arm64(program, 0, 6, output,
          PPC_TO_ARM64_MAX_WORDS, &output_count) ==
          PPC_TO_ARM64_INVALID_ARGUMENT);
    CHECK(ppc_translate_to_arm64(program, 4, 18, output,
          PPC_TO_ARM64_MAX_WORDS, &output_count) ==
          PPC_TO_ARM64_INVALID_ARGUMENT);

    puts("PASS: bounded PowerPC-to-ARM64 translation and rejection tests");
    return 0;
}
