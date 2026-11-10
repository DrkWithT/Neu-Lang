#pragma once

#include "shape.h"
#include "value.h"



typedef struct nl_table_t {
    shape* layout;
    value* pvals;
    value* ivals;
    uint32_t prop_n;        // property value count
    uint32_t prop_cap;      // property capacity
    uint32_t item_n;        // item value count
    uint32_t item_cap;      // item capacity
} table;
