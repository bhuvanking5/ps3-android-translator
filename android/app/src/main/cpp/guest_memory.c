#include "guest_memory.h"

#include <limits.h>

void ps3_guest_memory_init(ps3_guest_memory *memory) {
    if (memory == 0) return;
    memory->count = 0u;
}

int ps3_guest_memory_map(ps3_guest_memory *memory, uint32_t base,
    uint32_t size, uint8_t *bytes, unsigned permissions) {
    if (memory == 0 || bytes == 0 || size == 0u) return 0;
    if ((permissions & ~(PS3_GUEST_MEMORY_READ | PS3_GUEST_MEMORY_WRITE |
                         PS3_GUEST_MEMORY_EXECUTE)) != 0u ||
        permissions == 0u) return 0;
    if ((uint64_t)base + (uint64_t)size > (uint64_t)UINT32_MAX + 1u)
        return 0;
    if (memory->count >= PS3_GUEST_MEMORY_MAX_REGIONS) return 0;

    const uint64_t new_start = base;
    const uint64_t new_end = new_start + size;
    for (size_t i = 0; i < memory->count; ++i) {
        const ps3_guest_memory_region *region = &memory->regions[i];
        const uint64_t start = region->base;
        const uint64_t end = start + region->size;
        if (new_start < end && start < new_end) return 0;
    }

    ps3_guest_memory_region *region = &memory->regions[memory->count++];
    region->base = base;
    region->size = size;
    region->bytes = bytes;
    region->permissions = permissions;
    return 1;
}

static ps3_guest_memory_region *find_region(ps3_guest_memory *memory,
    uint32_t address, unsigned permission, uint32_t width) {
    if (memory == 0) return 0;
    for (size_t i = 0; i < memory->count; ++i) {
        ps3_guest_memory_region *region = &memory->regions[i];
        if ((region->permissions & permission) == 0u) continue;
        if (address >= region->base &&
            (uint64_t)address + width <= (uint64_t)region->base + region->size)
            return region;
    }
    return 0;
}

int ps3_guest_memory_read32(void *context, uint32_t address, uint32_t *value) {
    if (context == 0 || value == 0 || (address & 3u) != 0u) return 0;
    ps3_guest_memory *memory = (ps3_guest_memory *)context;
    ps3_guest_memory_region *region =
        find_region(memory, address, PS3_GUEST_MEMORY_READ, 4u);
    if (region == 0) return 0;
    const size_t offset = (size_t)(address - region->base);
    const uint8_t *p = region->bytes + offset;
    *value = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
             ((uint32_t)p[2] << 8) | (uint32_t)p[3];
    return 1;
}

int ps3_guest_memory_fetch32(void *context, uint32_t address, uint32_t *value) {
    if (context == 0 || value == 0 || (address & 3u) != 0u) return 0;
    ps3_guest_memory *memory = (ps3_guest_memory *)context;
    ps3_guest_memory_region *region =
        find_region(memory, address, PS3_GUEST_MEMORY_EXECUTE, 4u);
    if (region == 0) return 0;
    const size_t offset = (size_t)(address - region->base);
    const uint8_t *p = region->bytes + offset;
    *value = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
             ((uint32_t)p[2] << 8) | (uint32_t)p[3];
    return 1;
}

int ps3_guest_memory_write32(void *context, uint32_t address, uint32_t value) {
    if (context == 0 || (address & 3u) != 0u) return 0;
    ps3_guest_memory *memory = (ps3_guest_memory *)context;
    ps3_guest_memory_region *region =
        find_region(memory, address, PS3_GUEST_MEMORY_WRITE, 4u);
    if (region == 0) return 0;
    const size_t offset = (size_t)(address - region->base);
    uint8_t *p = region->bytes + offset;
    p[0] = (uint8_t)(value >> 24);
    p[1] = (uint8_t)(value >> 16);
    p[2] = (uint8_t)(value >> 8);
    p[3] = (uint8_t)value;
    return 1;
}

int ps3_guest_memory_read64(void *context, uint32_t address, uint64_t *value) {
    if (context == 0 || value == 0 || (address & 7u) != 0u) return 0;
    ps3_guest_memory *memory = (ps3_guest_memory *)context;
    ps3_guest_memory_region *region =
        find_region(memory, address, PS3_GUEST_MEMORY_READ, 8u);
    if (region == 0) return 0;
    const uint8_t *p = region->bytes + (size_t)(address - region->base);
    *value = ((uint64_t)p[0] << 56) | ((uint64_t)p[1] << 48) |
             ((uint64_t)p[2] << 40) | ((uint64_t)p[3] << 32) |
             ((uint64_t)p[4] << 24) | ((uint64_t)p[5] << 16) |
             ((uint64_t)p[6] << 8) | (uint64_t)p[7];
    return 1;
}

int ps3_guest_memory_write64(void *context, uint32_t address, uint64_t value) {
    if (context == 0 || (address & 7u) != 0u) return 0;
    ps3_guest_memory *memory = (ps3_guest_memory *)context;
    ps3_guest_memory_region *region =
        find_region(memory, address, PS3_GUEST_MEMORY_WRITE, 8u);
    if (region == 0) return 0;
    uint8_t *p = region->bytes + (size_t)(address - region->base);
    p[0] = (uint8_t)(value >> 56); p[1] = (uint8_t)(value >> 48);
    p[2] = (uint8_t)(value >> 40); p[3] = (uint8_t)(value >> 32);
    p[4] = (uint8_t)(value >> 24); p[5] = (uint8_t)(value >> 16);
    p[6] = (uint8_t)(value >> 8); p[7] = (uint8_t)value;
    return 1;
}
