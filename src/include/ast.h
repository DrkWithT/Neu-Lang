#pragma once

#include "strs.h"
#include "dynbuf.h"
#include "token.h"



typedef enum nl_ast_tag_t : uint8_t {
    nl_ast_invalid,
    nl_ast_literal,
    nl_ast_literal_array,
    nl_ast_literal_table,
    nl_ast_expr_func,
    nl_ast_expr_access,
    nl_ast_expr_call,
    nl_ast_unary,
    nl_ast_binary_normal,
    nl_ast_binary_logical,
    nl_ast_expr_assign,
    nl_ast_cond,
    nl_ast_loop,
    nl_ast_loop_abator,
    nl_ast_stmt_def,
    nl_ast_block,
    nl_ast_stmt_ret,
    nl_ast_expr_stmt,
} ast_tag;

typedef enum nl_ast_op : uint8_t {
    nl_ast_op_none,
    nl_ast_op_nullish,
    nl_ast_op_negate,
    nl_ast_op_logical_negate,
    nl_ast_op_coalesce,
    nl_ast_op_mod,
    nl_ast_op_mul,
    nl_ast_op_div,
    nl_ast_op_add,
    nl_ast_op_sub,
    nl_ast_op_equals,
    nl_ast_op_bang_equals,
    nl_ast_op_lt,
    nl_ast_op_gt,
    nl_ast_op_lte,
    nl_ast_op_gte,
    nl_ast_op_bang,
    nl_ast_op_and,
    nl_ast_op_or,
    nl_ast_op_bind,
    nl_ast_op_bind_mul,
    nl_ast_op_bind_div,
    nl_ast_op_bind_add,
    nl_ast_op_bind_sub,
} ast_op;

typedef struct nl_ast_node_t ast_node;

typedef void(*nl_ast_node_deleter_t)(void* node);



typedef struct nl_ast_literal_t {
    tk token;
} literal;

typedef struct nl_ast_literal_array_t {
    dynbuf items;   // stores dynamic array of `ast_node`
} array;

typedef struct nl_ast_table_prop_t {
    ast_node* key;
    ast_node* expr;
} table_prop;

typedef struct nl_ast_literal_table_t {
    dynbuf props;   // stores dynamic array of `ast_node`
} table;

typedef struct nl_ast_expr_func_t {
    dynbuf param_tkv;   // token vector
    ast_node* body;
} expr_func;

/**
 ### ABOUT
 - Utility type for `expr_access` representing `.name` or `[<expr>]` access on a metatable.
*/
typedef struct nl_ast_part_access_t {
    union {
        ast_node* comp_key;
        uint32_t name_tpos;
    } data;
    bool is_named;
} part_access;

typedef struct nl_ast_expr_access_t {
    dynbuf accesses;
    ast_node* target;
} expr_access;

typedef struct nl_ast_expr_call_t {
    dynbuf arg_exprs;   // vector of nodes on heap
    ast_node* callee;
} expr_call;

typedef struct nl_ast_expr_unary_t {
    ast_node* inner;
    ast_op op;
} expr_unary;

typedef struct nl_ast_expr_binary_normal_t {
    ast_node* lhs;
    ast_node* rhs;
    ast_op op;
    bool is_ltr;
} expr_binary_normal;

typedef struct nl_ast_expr_binary_logical_t {
    ast_node* lhs;
    ast_node* rhs;
    ast_op op;
} expr_binary_logical;

typedef struct nl_ast_expr_assign_t {
    ast_node* dest;
    ast_node* src;
    ast_op op;
} expr_assign;

/**
 ### ABOUT
 - Utility type for `stmt_cond` or `expr_cond` representing `<cond-expr> -> <body>` per conditional case.
*/
typedef struct nl_ast_stmt_cond_entry_t {
    ast_node* check;    // ! `nullptr` if last / wildcard case applies.
    ast_node* body;
} cond_entry;

typedef struct nl_ast_cond_t {
    dynbuf entries;     // vector of `cond_entry` values
    bool is_expr;
} cond;

typedef struct nl_ast_loop_t {
    ast_node* init_expr;
    ast_node* check_expr;
    ast_node* update_expr;
    ast_node* body;
    ast_node* result_expr;
} loop;

typedef struct nl_loop_abator_t {
    ast_node* inner;    // `nullptr` on `continue;` or not `nullptr` on `break <expr>;`
} loop_abator;

typedef struct nl_ast_stmt_def_t {
    dynbuf bindings;    // `def_binding` vector
} stmt_def;

typedef struct nl_ast_def_binding_t {
    tk name;
    ast_node* init;
} def_binding;

typedef struct nl_ast_block_t {
    dynbuf items;   // `ast_node` vector
    bool is_expr;   // blocks can be expressions or statements (prefixed with `do`)
} block;

typedef struct nl_ast_stmt_ret_t {
    ast_node* result;
} stmt_ret;

typedef struct nl_expr_stmt_t {
    ast_node* expr;
} expr_stmt;

typedef struct nl_ast_node_t {
    union {
        uint8_t dud;
        literal lit;
        array arr;
        table tbl;
        expr_func fun;
        expr_access accs;
        expr_call call;
        expr_unary una;
        expr_binary_normal bin_n;
        expr_binary_logical bin_l;
        expr_assign ass;
        cond cnd;
        loop lp;
        loop_abator lab;
        stmt_def def;
        block bl;
        stmt_ret sr;
        expr_stmt es;
    } data;             // varying types of syntactical data are stored in this space
    ast_tag tag;        // indicates which kind of AST data is stored, corresponding to the ast_deleter function per node type
} ast_node;



[[nodiscard]]
literal nl_ast_literal_new(tk token);

void nl_ast_table_prop_del(void* self);
void nl_ast_part_access_del(void* self);
void nl_ast_cond_entry_del(void* self);
void nl_ast_def_binding_del(void* self);
void ast_node_del(void* node);



typedef struct nl_ast_unit_t {
    dynbuf roots;       // stores `ast_node[]`
    str source;         // stores view of original source string
} ast_unit;

void ast_unit_del(void* self);
