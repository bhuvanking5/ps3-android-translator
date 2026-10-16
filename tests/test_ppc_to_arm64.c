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

static uint32_t branch(int displacement, unsigned absolute, unsigned link) {
    return (18u << 26) | ((uint32_t)displacement & 0x03FFFFFCu) |
           ((absolute & 1u) << 1) | (link & 1u);
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

    CHECK(ppc_translate_to_arm64(program, 4, 6, output,
          PPC_TO_ARM64_MAX_WORDS, &output_count) == PPC_TO_ARM64_OK);
    CHECK(output_count > 4u * 4u);
    CHECK(output[0] == (0xD1000000u | (256u << 10) | (31u << 5) | 31u));
    CHECK(output[1] == 0xD2800010u); /* movz x16, #0 for GPR initialization */
    CHECK(output[output_count - 3] ==
          (0xF9400000u | (6u << 10) | (31u << 5))); /* load result into x0 */
    CHECK(output[output_count - 2] ==
          (0x91000000u | (256u << 10) | (31u << 5) | 31u)); /* restore SP */
    CHECK(output[output_count - 1] == 0xD65F03C0u); /* ret */

    const uint32_t negative[] = { addi(3, 0, -2) };
    CHECK(ppc_translate_to_arm64(negative, 1, 3, output,
          PPC_TO_ARM64_MAX_WORDS, &output_count) == PPC_TO_ARM64_OK);
    CHECK(output[34] == 0x92800030u); /* movn x16, #1 => -2 */
    CHECK(output[output_count - 3] ==
          (0xF9400000u | (3u << 10) | (31u << 5)));

    /* Large signed immediates now use MOVZ/MOVN + register arithmetic. */
    const uint32_t large_immediate[] = { addi(31, 2, 12000) };
    CHECK(ppc_translate_to_arm64(large_immediate, 1, 31, output,
          PPC_TO_ARM64_MAX_WORDS, &output_count) == PPC_TO_ARM64_OK);

    /* Local relative branches are patched after translated instruction offsets are known. */
    const uint32_t forward_branch[] = {
        branch(8, 0, 0), addi(3, 0, 1), addi(4, 0, 2)
    };
    CHECK(ppc_translate_to_arm64(forward_branch, 3, 4, output,
          PPC_TO_ARM64_MAX_WORDS, &output_count) == PPC_TO_ARM64_OK);
    CHECK(output[34] == 0x14000003u); /* B skips the two-word ADDI and lands at the next block */

    const uint32_t backward_branch[] = {
        addi(3, 0, 1), addi(4, 0, 2), branch(-4, 0, 0)
    };
    CHECK(ppc_translate_to_arm64(backward_branch, 3, 4, output,
          PPC_TO_ARM64_MAX_WORDS, &output_count) == PPC_TO_ARM64_OK);
    CHECK(output[38] == 0x17FFFFFEu); /* B from word 38 back to the previous ADDI */

    const uint32_t out_of_range_branch[] = { branch(8, 0, 0), addi(3, 0, 1) };
    CHECK(ppc_translate_to_arm64(out_of_range_branch, 2, 3, output,
          PPC_TO_ARM64_MAX_WORDS, &output_count) ==
          PPC_TO_ARM64_UNSUPPORTED_INSTRUCTION);
    CHECK(output_count == 0);
    const uint32_t linked_branch[] = { branch(4, 0, 1), addi(3, 0, 1) };
    CHECK(ppc_translate_to_arm64(linked_branch, 2, 3, output,
          PPC_TO_ARM64_MAX_WORDS, &output_count) ==
          PPC_TO_ARM64_UNSUPPORTED_INSTRUCTION);

    const uint32_t unsupported[] = { 0x40000004u };
    CHECK(ppc_translate_to_arm64(unsupported, 1, 3, output,
          PPC_TO_ARM64_MAX_WORDS, &output_count) ==
          PPC_TO_ARM64_UNSUPPORTED_INSTRUCTION);
    CHECK(output_count == 0);

    CHECK(ppc_translate_to_arm64(program, 4, 6, output, 5,
          &output_count) == PPC_TO_ARM64_OUTPUT_TOO_SMALL);
    CHECK(output_count == 0);
    CHECK(ppc_translate_to_arm64(program, 0, 6, output,
          PPC_TO_ARM64_MAX_WORDS, &output_count) ==
          PPC_TO_ARM64_INVALID_ARGUMENT);
    CHECK(ppc_translate_to_arm64(program, 4, 32, output,
          PPC_TO_ARM64_MAX_WORDS, &output_count) ==
          PPC_TO_ARM64_INVALID_ARGUMENT);

    puts("PASS: stack-backed register mapping, translation, and rejection tests");
    return 0;
}
