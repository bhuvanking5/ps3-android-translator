#include <stdio.h>
#include <stdint.h>
#include "prototype_runtime.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "CHECK failed at line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void) {
    uint64_t registers[32] = {0};
    const uint32_t program[] = {
        (14u << 26) | (3u << 21) | 7u, /* addi r3,r0,7 */
        (14u << 26) | (4u << 21) | (3u << 16) | 5u /* addi r4,r3,5 */
    };
    ps3_controller_report report;

    ps3_controller_input_publish(PS3_INPUT_CROSS | PS3_INPUT_L1,
        -1.0f, 0.5f, 1.0f, 0.0f);
    CHECK(ps3_prototype_run_slice(program, 2u, registers, &report) == PPC_CPU_OK);
    CHECK(registers[3] == 7u && registers[4] == 12u);
    CHECK(report.buttons == (PS3_INPUT_CROSS | PS3_INPUT_L1));
    CHECK(report.left_x == 0u && report.left_y == 191u);
    CHECK(report.right_x == 255u && report.right_y == 128u);

    CHECK(ps3_prototype_run_slice(program, 2u, registers, 0) ==
          PPC_CPU_INVALID_ARGUMENT);
    puts("Prototype runtime integration tests passed.");
    return 0;
}
