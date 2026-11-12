#pragma once

#include "strs.h"
#include "dynbuf.h"
#include "ast.h"
#include "bytecode.h"



typedef enum nl_bcc_symtag_t : uint8_t {
    nl_bcc_sym_invalid,
    nl_bcc_sym_unknown,
    nl_bcc_sym_k,           // constant `value`
    nl_bcc_sym_global_k,
    nl_bcc_sym_local_r,
    nl_bcc_sym_foreign_key
} symtag;

typedef struct nl_bcc_syminfo_t {
    str name;
    int32_t n;
    uint16_t cid;
    symtag tag;
} syminfo;

typedef enum nl_bcc_flag_t {
    nl_bcc_cf_ok = 1,
    nl_bcc_cf_in_of_lhs = 2,
    nl_bcc_cf_got_temp = 4
} bcc_flag;

typedef struct nl_bcc_result_t {
    int32_t n;
    uint16_t flags;
    symtag domain;
} bcc_result;

typedef struct nl_bcc_loopinfo_t {
    dynbuf rep_iposv;   // continue jump locations
    dynbuf exit_iposv;  // break jump locations
} bcc_loopinfo;

typedef struct nl_bcc_symscope_t {
    dynbuf infos;       // vector of `syminfo`s
    str name;
    uint16_t top_konst; // next constant ID
    uint16_t next_ic;   // next IC ID
    uint16_t cid;       // associated chunk ID of scope's function
} symscope;

typedef struct nl_bcc_state_t {
    dynbuf scopes;          // scope stack
    dynbuf loops;           // loop info stack with `bcc_loopinfo`s
    ent_pool pool_s;        // interned strings
    ent_pool pool_o;        // objects
    ent_pool pool_c;        // closures
    dynbuf chunks;          // bytecode chunks
    value* gvals;           // pre-allocated superglobal array
    uint16_t gvals_cap;
    uint16_t gvals_n;
    uint16_t top_reg;   // next register (stack allocated position)
    uint16_t curr_cid;      // currently emitting chunk ID
    uint16_t entry_cid;     // main chunk ID (`0`)
    uint16_t errors;
    uint16_t line;
    uint16_t col;
} bcc_state;

[[nodiscard]]
bcc_state nl_bcc_state_new(int32_t max_strings, int32_t max_objects, int32_t max_closures);

void nl_bcc_state_del(bcc_state* bcc);

// ent_handle_mut nl_bcc_record_global(const str* name, const table* object); // todo: implement upon fully supporting tables

// void nl_bcc_record_global_val(const str* name, const value* gv); // todo: implement upon fully supporting global funcs

[[nodiscard]]
program nl_bcc_compile_tu(const ast_unit* tu);
