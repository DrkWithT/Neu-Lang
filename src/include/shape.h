#pragma once

#include "dynbuf.h"



typedef struct nl_shape_ent_t {
    int32_t key;        // holds `ID` to name
    uint16_t offset;    // holds property value offset
} shape_ent;

typedef struct nl_shape_t {
    dynbuf entries;     // stores `shape_ent[]`
    void* proto;        // stores pointer to trait prototype object
} shape;
