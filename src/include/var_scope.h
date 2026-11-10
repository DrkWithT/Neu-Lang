#pragma once

#include "dynbuf.h"



typedef enum nl_var_type_t : uint8_t {
    nl_var_error,   // non-resolvable location
    nl_var_global,  // only globally available
    nl_var_local,   // only function available
    nl_var_capture  // closure-only (upvalue)
} var_type;

typedef struct nl_var_ent_t {
    uint32_t id;
    var_type tag;
} var_ent;

typedef struct nl_scope_info_t {
    dynbuf vars;                // vector of `var_ent`, has `nullptr` deleter
    int32_t parent_scope_id;    // parent scope in scopes pool
    bool persist;               // `true` for persistent closures
} scope;

void scope_del(void* s);
