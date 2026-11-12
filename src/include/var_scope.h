#pragma once

#include "value.h"


typedef enum nl_var_type_t : uint8_t {
    nl_var_error,       // non-resolvable location
    nl_var_global,      // only globally available
    nl_var_local,       // only function available
    nl_var_uv_o,        // open upvalue
    nl_var_uv_c,        // closed upvalue
} var_type;

typedef struct nl_var_ent_t {
    uint32_t id;    // offset / name ID
    uint16_t cid;   // variable's home chunk ID
    var_type tag;   // tag for distinguishing locals, globals, etc.
} var_ent;

typedef struct nl_scope_info_t scope;

typedef struct nl_scope_info_t {
    const var_ent* vars;        // viewing window of `chunks[cid].vinfo`, has `nullptr` deleter
    scope* parent;              // parent scope in scopes pool
    value* vbase;               // variable window
    value* closed_uvbuf;        // closed upvalue buffer
    uint16_t cid;               // corresponding function's chunk ID (`cid`)
    uint16_t vars_n;             // count of valid slots in `vars`
    bool saved;                 // is `closed_uvbuf` used?
} scope;

void scope_del(void* s);
