#include "guest_relocations.h"

#include <limits.h>

static int add_signed(uint64_t base, int64_t addend, uint64_t *result) {
    if (addend >= 0) {
        const uint64_t amount = (uint64_t)addend;
        if (base > UINT64_MAX - amount) return 0;
        *result = base + amount;
        return 1;
    }
    /* Avoid negating INT64_MIN in signed arithmetic. */
    const uint64_t amount = (uint64_t)(-(addend + 1)) + 1u;
    if (base < amount) return 0;
    *result = base - amount;
    return 1;
}

static int target_is_writable(const ps3_guest_memory *memory,
                              uint32_t address) {
    if ((address & 3u) != 0u || address > UINT32_MAX - 3u) return 0;
    for (size_t i = 0; i < memory->count; ++i) {
        const ps3_guest_memory_region *region = &memory->regions[i];
        const uint64_t end = (uint64_t)region->base + region->size;
        if ((region->permissions & PS3_GUEST_MEMORY_WRITE) != 0u &&
            address >= region->base && (uint64_t)address + 4u <= end) {
            return 1;
        }
    }
    return 0;
}

ps3_guest_reloc_status ps3_guest_relocations_apply(
    ps3_guest_memory *memory,
    const ps3_guest_relocation *records,
    size_t record_count) {
    if (memory == 0 || (records == 0 && record_count != 0u)) {
        return PS3_GUEST_RELOC_INVALID_ARGUMENT;
    }
    if (record_count > PS3_GUEST_RELOCATION_MAX_RECORDS) {
        return PS3_GUEST_RELOC_TOO_MANY_RECORDS;
    }

    uint32_t staged[PS3_GUEST_RELOCATION_MAX_RECORDS];
    for (size_t i = 0; i < record_count; ++i) {
        const ps3_guest_relocation *r = &records[i];
        if (!target_is_writable(memory, r->target_address)) {
            return PS3_GUEST_RELOC_TARGET_NOT_WRITABLE;
        }

        uint64_t sum = 0u;
        if (!add_signed(r->symbol_value, r->addend, &sum)) {
            return PS3_GUEST_RELOC_VALUE_OUT_OF_RANGE;
        }
        if (r->type == PS3_PPC64_R_ADDR32) {
            if (sum > UINT32_MAX) return PS3_GUEST_RELOC_VALUE_OUT_OF_RANGE;
            staged[i] = (uint32_t)sum;
        } else if (r->type == PS3_PPC64_R_REL32) {
            if (sum > (uint64_t)INT64_MAX) {
                return PS3_GUEST_RELOC_VALUE_OUT_OF_RANGE;
            }
            const int64_t relative = (int64_t)sum -
                                     (int64_t)r->target_address;
            if (relative < INT32_MIN || relative > INT32_MAX) {
                return PS3_GUEST_RELOC_VALUE_OUT_OF_RANGE;
            }
            staged[i] = (uint32_t)(int32_t)relative;
        } else {
            return PS3_GUEST_RELOC_UNSUPPORTED_TYPE;
        }
    }

    /* Every operation below was prevalidated; writes are big-endian and
     * should not fail unless the memory map is concurrently mutated. */
    for (size_t i = 0; i < record_count; ++i) {
        if (!ps3_guest_memory_write32(memory, records[i].target_address,
                                      staged[i])) {
            return PS3_GUEST_RELOC_TARGET_NOT_WRITABLE;
        }
    }
    return PS3_GUEST_RELOC_OK;
}
