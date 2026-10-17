#ifndef PS3_GUEST_MEMORY_H
#define PS3_GUEST_MEMORY_H

#include <stddef.h>
#include <stdint.h>

#define PS3_GUEST_MEMORY_MAX_REGIONS 8u

enum {
    PS3_GUEST_MEMORY_READ = 1u,
    PS3_GUEST_MEMORY_WRITE = 2u,
    PS3_GUEST_MEMORY_EXECUTE = 4u
};

typedef struct {
    uint32_t base;
    uint32_t size;
    uint8_t *bytes;
    unsigned permissions;
} ps3_guest_memory_region;

typedef struct {
    ps3_guest_memory_region regions[PS3_GUEST_MEMORY_MAX_REGIONS];
    size_t count;
} ps3_guest_memory;

void ps3_guest_memory_init(ps3_guest_memory *memory);
int ps3_guest_memory_map(ps3_guest_memory *memory, uint32_t base,
    uint32_t size, uint8_t *bytes, unsigned permissions);
int ps3_guest_memory_read32(void *context, uint32_t address, uint32_t *value);
int ps3_guest_memory_fetch32(void *context, uint32_t address, uint32_t *value);
int ps3_guest_memory_write32(void *context, uint32_t address, uint32_t value);
int ps3_guest_memory_read8(void *context, uint32_t address, uint8_t *value);
int ps3_guest_memory_read16(void *context, uint32_t address, uint16_t *value);
int ps3_guest_memory_write8(void *context, uint32_t address, uint8_t value);
int ps3_guest_memory_write16(void *context, uint32_t address, uint16_t value);
int ps3_guest_memory_read64(void *context, uint32_t address, uint64_t *value);
int ps3_guest_memory_write64(void *context, uint32_t address, uint64_t value);

#endif
