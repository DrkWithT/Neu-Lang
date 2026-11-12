#pragma once

#include "dynbuf.h"
#include "pool.h"
#include "value.h"
#include "ic.h"



typedef enum nl_op_t : uint8_t {
    nl_op_nop,          // pc++
    nl_op_store_nil,
    nl_op_store_true,
    nl_op_store_false,
    nl_op_swap_r,       // swap dest src
    nl_op_loadi_u16,    // mov dest (i32)u16-imm
    nl_op_mov_rk,       // mov dest constant-id
    nl_op_mov_rr,       // mov dest src
    nl_op_ld_upval,     // load_upval dest <upval-ID>
    nl_op_sto_upval,    // store_upval <upval-ID> src
    nl_op_test_nil,     // test_nil dest
    nl_op_neg_num,      // numeric_neg dest
    nl_op_neg_log,      // logical_neg dest
    nl_op_mod,          // op_xxx dest a1 a2...
    nl_op_mul,
    nl_op_div,
    nl_op_add,
    nl_op_sub,
    nl_op_lt,
    nl_op_lte,
    nl_op_gt,
    nl_op_gte,
    nl_op_eq,
    nl_op_ne,
    nl_op_jmp_nil,      // jmp_nil src pc
    nl_op_jmp_if,       // jmp_if src pc
    nl_op_jmp_else,     // jmp_else src pc
    nl_op_jmp,          // jmp pc
    nl_op_mk_closure,   // mk_closure dest
    nl_op_call,         // call dest-reg func-reg argc
    nl_op_call_n,       // call_native dest-reg native-id-imm argc
    nl_op_ret,          // ret src
} op;

typedef struct nl_chunk_t {
    dynbuf vinfo;       // buffer of variable locators (no deleter)
    uint8_t* bc;        // bytecode
    value* kd;          // constants data
    ic* icv;            // property-access inline caches
    uint16_t code_len;  // `inst` count
    uint16_t code_cap;  // `inst` capacity
    uint16_t k_len;
    uint16_t k_cap;
    uint16_t ic_len;
    uint16_t ic_cap;
    uint16_t regs_n;
    uint8_t arity;
} chunk;

void chunk_del(void* c);

typedef struct nl_program_t {
    ent_pool pool_s;    // interned strings
    ent_pool pool_o;    // objects
    ent_pool pool_c;    // closures
    dynbuf chunks;
    value* gv;          // global values buffer
    uint16_t gc_len;
    uint16_t gc_cap;
    uint16_t entry_cid;
} program;

void program_del(void* pg);
