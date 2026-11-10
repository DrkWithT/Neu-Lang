#pragma once

#include <stddef.h>
#include <stdint.h>



typedef enum nl_vtag_t {
    nl_vtag_nil,
    nl_vtag_bool,
    nl_vtag_int,
    nl_vtag_num,
    nl_vtag_str,
    nl_vtag_obj
} vtag;

typedef struct nl_value_t {
    union {
        uint8_t b;
        int32_t n;
        float f;
    };
    vtag tag;
} value;
