#include "prototype_guest_io.h"

/*
 * Minimal, read-only controller MMIO-shaped adapter for future CPU integration.
 * It does not map guest RAM, alter CPU execution, or model real PS3 registers.
 */
ps3_guest_io_status ps3_prototype_guest_read32(
    const ps3_controller_report *report, uint32_t address, uint32_t *value) {
    if (report == 0 || value == 0) return PS3_GUEST_IO_INVALID_ARGUMENT;
    if (address < PS3_PROTOTYPE_CONTROLLER_BASE) {
        return PS3_GUEST_IO_UNMAPPED_ADDRESS;
    }

    const uint32_t offset = address - PS3_PROTOTYPE_CONTROLLER_BASE;
    if (offset >= PS3_PROTOTYPE_CONTROLLER_SPAN || (offset & 3u) != 0u) {
        return PS3_GUEST_IO_UNMAPPED_ADDRESS;
    }

    const uint32_t word_index = offset / 4u;
    const ps3_controller_word_status status =
        ps3_controller_report_read_word(report, word_index, value);
    return status == PS3_CONTROLLER_WORD_OK
        ? PS3_GUEST_IO_OK : PS3_GUEST_IO_UNMAPPED_ADDRESS;
}
