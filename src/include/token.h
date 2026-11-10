#pragma once

#include <stdint.h>
#include "strs.h"



typedef enum nl_tk_tag_t : uint8_t {
    nl_tk_unknown,
    nl_tk_spaces,
    nl_tk_comment,
    nl_tk_kw_def,
    nl_tk_kw_fun,
    nl_tk_kw_cond,
    nl_tk_kw_else,
    nl_tk_kw_loop,
    nl_tk_kw_break,
    nl_tk_kw_continue,
    nl_tk_kw_ret,
    nl_tk_kw_nil,
    nl_tk_kw_true,
    nl_tk_kw_false,
    nl_tk_identifier,
    nl_tk_lt_int,
    nl_tk_lt_num,
    nl_tk_lt_plain_str,
    nl_tk_lt_esc_str,
    nl_tk_op_nullish,
    nl_tk_op_coalesce,
    nl_tk_op_mod,
    nl_tk_op_mul,
    nl_tk_op_div,
    nl_tk_op_add,
    nl_tk_op_sub,
    nl_tk_op_equals,
    nl_tk_op_bang_equals,
    nl_tk_op_lt,
    nl_tk_op_gt,
    nl_tk_op_lte,
    nl_tk_op_gte,
    nl_tk_op_bang,
    nl_tk_op_and,
    nl_tk_op_or,
    nl_tk_op_bind,
    nl_tk_op_bind_mul,
    nl_tk_op_bind_div,
    nl_tk_op_bind_add,
    nl_tk_op_bind_sub,
    nl_tk_arrow,
    nl_tk_comma,
    nl_tk_colon,
    nl_tk_semicolon,
    nl_tk_lparen,
    nl_tk_rparen,
    nl_tk_lbrace,
    nl_tk_rbrace,
    nl_tk_lbrack,
    nl_tk_rbrack,
    nl_tk_eof,
} tk_tag;

typedef struct nl_tk_t {
    uint16_t begin;
    uint16_t n;
    uint16_t line;
    uint16_t col;
    tk_tag tag;
} tk;

[[nodiscard]]
static inline tk mk_token(uint16_t begin_, uint16_t n_, uint16_t line_, uint16_t col_, tk_tag tag_) {
    return (tk) {
        .begin = begin_,
        .n = n_,
        .line = line_,
        .col = col_,
        .tag = tag_
    };
}

[[nodiscard]]
static inline str tk_get_lexeme(const tk* self, const str* source) {
    return str_new(source->s + self->begin, self->n);
}

void tk_output(FILE* os, const tk* token);
