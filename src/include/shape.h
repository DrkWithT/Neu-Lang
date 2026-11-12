#pragma once

#include "dynbuf.h"



typedef enum nl_prop_flag_t : uint8_t {
    nl_prop_invalid = 0x1,
    nl_prop_writable = 0x2,
    nl_prop_may_del = 0x4,
    nl_prop_default = nl_prop_writable | nl_prop_may_del
} prop_flag;

typedef struct nl_shape_ent_t {
    int32_t key_sid;        // holds string `ID` of name
    uint16_t offset;        // holds property value offset
    uint8_t meta;          // holds property flags
} shape_ent;

typedef struct nl_shape_link_t {
    int32_t key_sid;
    int32_t shape_id;
    bool is_rev;        // for property deletion transitions
} shape_link;

typedef struct nl_shape_t {
    dynbuf entries;     // stores `shape_ent[]`
    void* proto;        // stores pointer to trait prototype object
    shape_link* links;  // transition edges to/from parent or child shapes, allowing shape reuse
    uint32_t link_cap;  // link capacity
    uint32_t link_n;    // link count
} shape;
