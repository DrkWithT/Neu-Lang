#pragma once

#include <stddef.h>
#include <stdint.h>



typedef enum nl_ic_state_t : uint8_t {
    nl_ics_idle,
    nl_ics_mono,
    nl_ics_dead,
} ic_state;

typedef struct nl_icache_t {
    const void* shape;
    void* receiver;
    size_t misses;
    int32_t key;
    uint16_t offset;
    ic_state state;
} ic;
