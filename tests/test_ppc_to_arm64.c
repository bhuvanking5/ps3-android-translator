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
    CHECK(output[0] == (0xD1000000u | (512u << 10) | (31u << 5) | 31u));
    CHECK(output[1] == 0xD2800010u); /* movz x16, #0 for GPR initialization */
    CHECK(output[output_count - 3] ==
          (0xF9400000u | (6u << 10) | (31u << 5))); /* load result into x0 */
    CHECK(output[output_count - 2] ==
          (0x91000000u | (512u << 10) | (31u << 5) | 31u)); /* restore SP */
    CHECK(output[output_count - 1] == 0xD65F03C0u); /* ret */

    const uint32_t negative[] = { addi(3, 0, -2) };
    CHECK(ppc_translate_to_arm64(negative, 1, 3, output,
          PPC_TO_ARM64_MAX_WORDS, &output_count) == PPC_TO_ARM64_OK);
    CHECK(output[66] == 0x92800030u); /* movn x16, #1 => -2 */
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
    CHECK(output[66] == 0x14000003u); /* B skips the two-word ADDI and lands at the next block */

    const uint32_t backward_branch[] = {
        addi(3, 0, 1), addi(4, 0, 2), branch(-4, 0, 0)
    };
    CHECK(ppc_translate_to_arm64(backward_branch, 3, 4, output,
          PPC_TO_ARM64_MAX_WORDS, &output_count) == PPC_TO_ARM64_OK);
    CHECK(output[70] == 0x17FFFFFEu); /* B from the relocated branch back to the previous ADDI */

    /* cmpwi plus bc emits signed compare, CR bits, and relocatable CBZ/CBNZ. */
    const uint32_t cmpwi = (11u << 26) | (3u << 16) | 7u;
    const uint32_t bc_equal = (16u << 26) | (12u << 21) |
                              (2u << 16) | 8u;
    const uint32_t conditional_branch[] = {
        addi(3, 0, 7), cmpwi, bc_equal, addi(4, 0, 99), addi(5, 0, 1)
    };
    CHECK(ppc_translate_to_arm64(conditional_branch, 5, 4, output,
          PPC_TO_ARM64_MAX_WORDS, &output_count) == PPC_TO_ARM64_OK);
    CHECK(output_count > 50u);
    /* The conditional branch is CBNZ x16 with a relocated imm19 field. */
    int found_conditional_branch = 0;
    for (size_t j = 0; j < output_count; ++j) {
        if ((output[j] & 0xFF00001Fu) == 0xB5000010u) {
            found_conditional_branch = 1;
            break;
        }
    }
    CHECK(found_conditional_branch);

    const uint32_t unsupported_bo[] = {
        (16u << 26) | (16u << 21) | (2u << 16) | 4u, addi(3, 0, 1)
    };
    CHECK(ppc_translate_to_arm64(unsupported_bo, 2, 3, output,
          PPC_TO_ARM64_MAX_WORDS, &output_count) ==
          PPC_TO_ARM64_UNSUPPORTED_INSTRUCTION);
    CHECK(output_count == 0);

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
