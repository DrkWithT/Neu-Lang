#include <stdio.h>
#include "parser.h"


static constexpr cond_entry nl_invalid_cond_entry_v = {
    .check = nullptr,
    .body = nullptr
};

static constexpr def_binding nl_invalid_def_bind_v = {
    .init = nullptr,
    .name = { .begin = 0, .n = 1, .line = 1, .col = 1, .tag = nl_tk_unknown }
};



[[nodiscard]]
ast_node* nl_parse_cond(parser* p, const str* source, bool is_expr);

[[nodiscard]]
ast_node* nl_parse_loop(parser* p, const str* source, bool is_expr);

[[nodiscard]]
ast_node* nl_parse_block(parser* p, const str* source, bool is_expr);

[[nodiscard]]
ast_node* nl_parse_next(parser* p, const str* source, bool is_expr);

[[nodiscard]]
ast_node* nl_make_ast_ptr(ast_node node) {
    auto p = malloc(sizeof(ast_node));

    memcpy(p, &node, sizeof(ast_node));

    return p;
}



[[nodiscard]]
bool parser_at_eof(const parser* p) {
    return p->curr.tag == nl_tk_eof;
}

[[nodiscard]]
tk parser_advance(parser* p, const str* source) {
    tk temp;

    do {
        temp = lexer_next(&p->tkgen, source);
    } while (temp.tag == nl_tk_spaces || temp.tag == nl_tk_comment);

    return temp;
}

void parser_consume(parser* p, const str* source) {
    p->prev = p->curr;
    p->curr = parser_advance(p, source);
}

[[nodiscard]]
bool parser_match(const parser* p, const tk_tag* tags, size_t n) {
    const auto curr_tk_tag = p->curr.tag;

    for (size_t tag_check_pos = 0; tag_check_pos < n; tag_check_pos++) {
        if (tags[tag_check_pos] == curr_tk_tag) {
            return true;
        }
    }

    return false;
}

[[nodiscard]]
bool parser_consume_of(parser* p, const str* source, const tk_tag* tags, size_t n) {
    if (parser_match(p, tags, n)) {
        p->prev = p->curr;
        p->curr = parser_advance(p, source);
        return true;
    }

    return false;
}

void parser_report_mini_error(parser* p, uint16_t error_line, uint16_t error_col, const char* error_msg) {
    p->errors++;
    fprintf(stdout, "Parse Error (\x1b[1;31m%d\x1b[0m) ~ \x1b[1;34msource\x1b[0m:\x1b[1;33m%d\x1b[0m:\x1b[1;33m%d\x1b[0m\n\t%s\n\n", p->errors,  error_line, error_col, error_msg);
}

// void parser_report_error(parser* p, const str* source, uint16_t sub_src_begin, uint16_t sub_src_end, uint16_t line, uint16_t error_line, const char* error_msg) {
//     const auto pv_source_p = source->s + sub_src_begin;
//     const auto pv_source_len = sub_src_end - sub_src_begin + 1;
//     auto pv_line = line;
//     char prev = '\n', curr = '\0';

//     p->errors++;
//     fprintf(stdout, "Syntax Error #\x1b[1;35m%d\x1b[0m:\n\n", p->errors);

//     for (size_t pv_offset = 0; pv_offset < pv_source_len; pv_offset++) {
//         if (prev == '\n') {
//             fprintf(stdout, "\x1b[1;35m%-5d\x1b[0m | ", pv_line);
//             pv_line++;
//         }

//         prev = curr;
//         curr = pv_source_p[pv_offset];

//         if (curr == '\n' && pv_line == error_line) {
//             fprintf(stdout, "\x1b[1;31m<--\x1b[0m %s", error_msg);
//         }

//         fputc(curr, stdout);
//     }

//     fputc('\n', stdout);
// }



[[nodiscard]]
ast_node* nl_parse_literal(parser* p, const str* source) {
    const auto curr_tk = p->curr;

    switch (curr_tk.tag) {
        case nl_tk_kw_nil:
        case nl_tk_kw_true:
        case nl_tk_kw_false:
        case nl_tk_identifier:
        case nl_tk_lt_int:
        case nl_tk_lt_num:
        case nl_tk_lt_plain_str:
        case nl_tk_lt_esc_str: {
            parser_consume(p, source);

            return nl_make_ast_ptr((ast_node) {
                .data.lit = nl_ast_literal_new(curr_tk),
                .tag = nl_ast_literal
            });
        }
        case nl_tk_kw_cond: return nl_parse_cond(p, source, true);
        case nl_tk_kw_loop: return nl_parse_loop(p, source, true);
        case nl_tk_lparen: {
            parser_consume(p, source);

            auto wrapped_expr = nl_parse_next(p, source, true);
            if (!wrapped_expr) {
                return nullptr;
            }

            if (!parser_consume_of(p, source, (tk_tag[]) { nl_tk_rparen }, 1)) {
                parser_report_mini_error(p, p->curr.line, p->curr.col, "Expected ')' closing this parenthesized expression.");

                ast_node_del(wrapped_expr);
                return nullptr;
            }

            return wrapped_expr;
        }
        case nl_tk_lbrace: return nl_parse_block(p, source, true);
        default: break;
    }

    parser_report_mini_error(p, curr_tk.line, curr_tk.col, "Unexpected token in literal but expected 'nil', 'true', 'false', number, string, or name.");

    return nullptr;
}

[[nodiscard]]
ast_node* nl_parse_literal_array(parser* p, const str* source) {
    return nullptr; // todo
}

[[nodiscard]]
table_prop nl_parse_table_prop(parser* p, const str* source) {
    return (table_prop) {
        .key = nullptr,
        .expr = nullptr
    }; // todo
}

[[nodiscard]]
ast_node* nl_parse_literal_table(parser* p, const str* source) {
    return nullptr; // todo
}

// ! Syntax Rules:
// ! <fun-expr> ::= "fun" <param-list> "->" <expr>
// ! <param-list> ::= "(" <identifier> ("," <identifier>)* ")"
[[nodiscard]]
ast_node* nl_parse_expr_func(parser* p, const str* source) {
    parser_consume(p, source); // ? eat prechecked 'fun'

    if (!parser_consume_of(p, source, (tk_tag[]) {nl_tk_lparen}, 1)) {
        parser_report_mini_error(p, p->curr.line, p->curr.col, "Expected '(' starting lambda parameters.");

        return nullptr;
    }

    auto param_tokens = dynbuf_new(sizeof(tk), 4, nullptr);

    if (!parser_match(p, (tk_tag[]) {nl_tk_rparen}, 1)) {
        if (!parser_consume_of(p, source, (tk_tag[]) {nl_tk_identifier}, 1)) {
            parser_report_mini_error(p, p->curr.line, p->curr.col, "Expected 1st parameter as a name.");

            dynbuf_del(&param_tokens);
            return nullptr;
        }
    }

    // ! ("," <identifier>)*
    while (!parser_at_eof(p)) {
        if (!parser_consume_of(p, source, (tk_tag[]) {nl_tk_comma }, 1)) {
            break;
        }

        if (!parser_consume_of(p, source, (tk_tag[]) {nl_tk_identifier}, 1)) {
            parser_report_mini_error(p, p->curr.line, p->curr.col, "Expected following parameter name.");

            dynbuf_del(&param_tokens);
            return nullptr;
        }

        const auto checked_name_tk = p->prev;
        dynbuf_push(&param_tokens, &checked_name_tk);
    }

    if (!parser_consume_of(p, source, (tk_tag[]) {nl_tk_rparen}, 1)) {
        parser_report_mini_error(p, p->curr.line, p->curr.col, "Expected ')' closing parameters.");

        dynbuf_del(&param_tokens);
        return nullptr;
    }

    if (!parser_consume_of(p, source, (tk_tag[]) { nl_tk_arrow }, 1)) {
        parser_report_mini_error(p, p->curr.line, p->curr.col, "Expected '->' before lambda body.");

        dynbuf_del(&param_tokens);
        return nullptr;
    }

    auto body_expr = nl_parse_next(p, source, true);
    if (!body_expr) {
        dynbuf_del(&param_tokens);
        return nullptr;
    }

    return nl_make_ast_ptr((ast_node) {
        .data.fun = {
            .param_tkv = param_tokens,
            .body = body_expr
        },
        .tag = nl_ast_expr_func
    });
}

[[nodiscard]]
part_access nl_parse_partial_access(parser* p, const str* source) {
    return (part_access) {
        .data.comp_key = nullptr,
        .is_named = false
    }; // todo
}

[[nodiscard]]
ast_node* nl_parse_expr_access(parser* p, const str* source) {
    return nullptr; // todo: implement this and use it in `nl_parse_expr_call`'s callee-expr.
}

[[nodiscard]]
ast_node* nl_parse_expr_call(parser* p, const str* source) {
    auto callee_ptr = nl_parse_literal(p, source); // ? see todo in `nl_parse_expr_access()`.
    if (!callee_ptr) {
        return nullptr;
    }

    if (!parser_consume_of(p, source, (tk_tag[]) {nl_tk_lparen}, 1)) {
        return callee_ptr; // ? no '(' means no argument list completes a call... just pass on the expression...
    }

    auto temp_args = dynbuf_new(sizeof(ast_node), 4, ast_node_del);

    if (!parser_match(p, (tk_tag[]) { nl_tk_rparen }, 1)) {
        auto first_arg_p = nl_parse_next(p, source, true);
        if (!first_arg_p) {
            dynbuf_del(&temp_args);
            ast_node_del(callee_ptr);
            return nullptr;
        }

        dynbuf_push(&temp_args, first_arg_p);
    }

    while (!parser_at_eof(p)) {
        if (!parser_consume_of(p, source, (tk_tag[]) {nl_tk_comma}, 1)) {
            break;
        }

        auto next_arg_ptr = nl_parse_next(p, source, true);
        if (!next_arg_ptr) {
            dynbuf_del(&temp_args);
            ast_node_del(callee_ptr);
            return nullptr;
        }

        dynbuf_push(&temp_args, next_arg_ptr);
    }

    if (!parser_consume_of(p, source, (tk_tag[]) {nl_tk_rparen}, 1)) {
        parser_report_mini_error(p, p->curr.line, p->curr.col, "Expected ')' closing args.");

        dynbuf_del(&temp_args);
        ast_node_del(callee_ptr);
        return nullptr;
    }

    return nl_make_ast_ptr((ast_node) {
        .data.call = {
            .arg_exprs = temp_args,
            .callee = callee_ptr
        },
        .tag = nl_ast_expr_call
    });
}

[[nodiscard]]
static inline ast_op tk_tag_to_unary_op(tk_tag t) {
    switch (t) {
        case nl_tk_op_nullish: return nl_ast_op_nullish;
        case nl_tk_op_sub: return nl_ast_op_negate;
        case nl_tk_op_bang: return nl_ast_op_logical_negate;
        default: return nl_ast_op_none;
    }
}

[[nodiscard]]
ast_node* nl_parse_unary_prefix(parser* p, const str* source) {
    const auto unary_ast_op = tk_tag_to_unary_op(p->curr.tag);

    if (unary_ast_op != nl_ast_op_none) {
        parser_consume(p, source);
    }

    auto inner_expr_ptr = nl_parse_expr_call(p, source);
    if (!inner_expr_ptr) {
        return nullptr;
    }

    if (unary_ast_op == nl_ast_op_none) {
        return inner_expr_ptr;
    }

    return nl_make_ast_ptr((ast_node) {
        .data.una = {
            .inner = inner_expr_ptr,
            .op = unary_ast_op
        },
        .tag = nl_ast_unary
    });
}

[[nodiscard]]
ast_node* nl_parse_coalesce(parser* p, const str* source) {
    ast_node* lhs_ptr = nl_parse_unary_prefix(p, source);    
    if (!lhs_ptr) {
        return nullptr;
    }

    ast_node* rhs_ptr = nullptr;

    while (!parser_at_eof(p)) {
        if (!parser_consume_of(p, source, (tk_tag[]) {nl_tk_op_coalesce}, 1)) {
            break;
        }

        rhs_ptr = nl_parse_unary_prefix(p, source);
        if (!rhs_ptr) {
            ast_node_del(lhs_ptr);
            return nullptr;
        }

        lhs_ptr = nl_make_ast_ptr((ast_node) {
            .data.bin_n = {
                .lhs = lhs_ptr,
                .rhs = rhs_ptr,
                .is_ltr = true,
                .op = nl_ast_op_coalesce,
            },
            .tag = nl_ast_binary_normal
        });
    }

    return lhs_ptr;
}

[[nodiscard]]
static inline ast_op tk_tag_to_factor_op(tk_tag t) {
    switch (t) {
        case nl_tk_op_mod: return nl_ast_op_mod;
        case nl_tk_op_mul: return nl_ast_op_mul;
        case nl_tk_op_div: return nl_ast_op_div;
        default: return nl_ast_op_none;
    }
}

[[nodiscard]]
ast_node* nl_parse_factor(parser* p, const str* source) {
    ast_node* lhs_ptr = nl_parse_coalesce(p, source);
    if (!lhs_ptr) {
        return nullptr;
    }

    ast_node* rhs_ptr = nullptr;

    while (!parser_at_eof(p)) {
        const auto factor_op = tk_tag_to_factor_op(p->curr.tag);

        if (factor_op == nl_ast_op_none) {
            break;
        }
        parser_consume(p, source);

        rhs_ptr = nl_parse_coalesce(p, source);
        if (!rhs_ptr) {
            ast_node_del(lhs_ptr);
            return nullptr;
        }

        lhs_ptr = nl_make_ast_ptr((ast_node) {
            .data.bin_n = {
                .lhs = lhs_ptr,
                .rhs = rhs_ptr,
                .is_ltr = true,
                .op = factor_op,
            },
            .tag = nl_ast_binary_normal
        });
    }

    return lhs_ptr;
}

[[nodiscard]]
static inline ast_op tk_tag_to_term_op(tk_tag t) {
    switch (t) {
        case nl_tk_op_add: return nl_ast_op_add;
        case nl_tk_op_sub: return nl_ast_op_sub;
        default: return nl_ast_op_none;
    }
}

[[nodiscard]]
ast_node* nl_parse_term(parser* p, const str* source) {
    ast_node* lhs_ptr = nl_parse_factor(p, source);
    if (!lhs_ptr) {
        return nullptr;
    }

    ast_node* rhs_ptr = nullptr;

    while (!parser_at_eof(p)) {
        const auto term_op = tk_tag_to_term_op(p->curr.tag);

        if (term_op == nl_ast_op_none) {
            break;
        }
        parser_consume(p, source);

        rhs_ptr = nl_parse_factor(p, source);
        if (!rhs_ptr) {
            ast_node_del(lhs_ptr);
            return nullptr;
        }

        lhs_ptr = nl_make_ast_ptr((ast_node) {
            .data.bin_n = {
                .lhs = lhs_ptr,
                .rhs = rhs_ptr,
                .is_ltr = true,
                .op = term_op,
            },
            .tag = nl_ast_binary_normal
        });
    }

    return lhs_ptr;
}

[[nodiscard]]
static inline ast_op tk_tag_to_compare_op(tk_tag t) {
    switch (t) {
        case nl_tk_op_lt: return nl_ast_op_lt;
        case nl_tk_op_gt: return nl_ast_op_gt;
        case nl_tk_op_lte: return nl_ast_op_lte;
        case nl_tk_op_gte: return nl_ast_op_gte;
        default: return nl_ast_op_none;
    }
}

[[nodiscard]]
ast_node* nl_parse_compare(parser* p, const str* source) {
    ast_node* lhs_ptr = nl_parse_term(p, source);
    if (!lhs_ptr) {
        return nullptr;
    }

    ast_node* rhs_ptr = nullptr;

    while (!parser_at_eof(p)) {
        const auto compare_op = tk_tag_to_compare_op(p->curr.tag);

        if (compare_op == nl_ast_op_none) {
            break;
        }
        parser_consume(p, source);

        rhs_ptr = nl_parse_term(p, source);
        if (!rhs_ptr) {
            ast_node_del(lhs_ptr);
            return nullptr;
        }

        lhs_ptr = nl_make_ast_ptr((ast_node) {
            .data.bin_n = {
                .lhs = lhs_ptr,
                .rhs = rhs_ptr,
                .is_ltr = true,
                .op = compare_op,
            },
            .tag = nl_ast_binary_normal
        });
    }

    return lhs_ptr;
}

[[nodiscard]]
static inline ast_op tk_tag_to_equality_op(tk_tag t) {
    switch (t) {
        case nl_tk_op_equals: return nl_ast_op_equals;
        case nl_tk_op_bang_equals: return nl_ast_op_bang_equals;
        default: return nl_ast_op_none;
    }
}

[[nodiscard]]
ast_node* nl_parse_equality(parser* p, const str* source) {
    ast_node* lhs_ptr = nl_parse_compare(p, source);
    if (!lhs_ptr) {
        return nullptr;
    }

    ast_node* rhs_ptr = nullptr;

    while (!parser_at_eof(p)) {
        const auto equality_op = tk_tag_to_equality_op(p->curr.tag);

        if (equality_op == nl_ast_op_none) {
            break;
        }
        parser_consume(p, source);

        rhs_ptr = nl_parse_compare(p, source);
        if (!rhs_ptr) {
            ast_node_del(lhs_ptr);
            return nullptr;
        }

        lhs_ptr = nl_make_ast_ptr((ast_node) {
            .data.bin_n = {
                .lhs = lhs_ptr,
                .rhs = rhs_ptr,
                .is_ltr = true,
                .op = equality_op,
            },
            .tag = nl_ast_binary_normal
        });
    }

    return lhs_ptr;
}

[[nodiscard]]
ast_node* nl_parse_logical_and(parser* p, const str* source) {
    ast_node* lhs_ptr = nl_parse_equality(p, source);
    if (!lhs_ptr) {
        return nullptr;
    }

    ast_node* rhs_ptr = nullptr;

    while (!parser_at_eof(p)) {
        if (!parser_consume_of(p, source, (tk_tag[]) { nl_tk_op_and }, 1)) {
            break;
        }

        rhs_ptr = nl_parse_equality(p, source);
        if (!rhs_ptr) {
            ast_node_del(lhs_ptr);
            return nullptr;
        }

        lhs_ptr = nl_make_ast_ptr((ast_node) {
            .data.bin_l = {
                .lhs = lhs_ptr,
                .rhs = rhs_ptr,
                .op = nl_ast_op_and,
            },
            .tag = nl_ast_binary_logical
        });
    }

    return lhs_ptr;
}

[[nodiscard]]
ast_node* nl_parse_logical_or(parser* p, const str* source) {
    ast_node* lhs_ptr = nl_parse_logical_and(p, source);
    if (!lhs_ptr) {
        return nullptr;
    }

    ast_node* rhs_ptr = nullptr;

    while (!parser_at_eof(p)) {
        if (!parser_consume_of(p, source, (tk_tag[]) { nl_tk_op_or }, 1)) {
            break;
        }

        rhs_ptr = nl_parse_logical_and(p, source);
        if (!rhs_ptr) {
            ast_node_del(lhs_ptr);
            return nullptr;
        }

        lhs_ptr = nl_make_ast_ptr((ast_node) {
            .data.bin_l = {
                .lhs = lhs_ptr,
                .rhs = rhs_ptr,
                .op = nl_ast_op_or,
            },
            .tag = nl_ast_binary_logical
        });
    }

    return lhs_ptr;
}

[[nodiscard]]
static inline ast_op tk_tag_to_assign_op(tk_tag t) {
    switch (t) {
        case nl_tk_op_bind: return nl_ast_op_bind;
        case nl_tk_op_bind_mul: return nl_ast_op_bind_mul;
        case nl_tk_op_bind_div: return nl_ast_op_bind_div;
        case nl_tk_op_bind_add: return nl_ast_op_bind_add;
        case nl_tk_op_bind_sub: return nl_ast_op_bind_sub;
        default: return nl_ast_op_none;
    }
}

[[nodiscard]]
ast_node* nl_parse_assignment(parser* p, const str* source) {
    ast_node* dest_ptr = nl_parse_logical_or(p, source);
    if (!dest_ptr) {
        return nullptr;
    }

    const auto assignment_op = tk_tag_to_assign_op(p->curr.tag);

    if (assignment_op == nl_ast_op_none) {
        return dest_ptr;
    }
    parser_consume(p, source);

    ast_node* src_ptr = nl_parse_assignment(p, source);
    if (!src_ptr) {
        ast_node_del(dest_ptr);
        return nullptr;
    }

    return nl_make_ast_ptr((ast_node) {
        .data.ass = {
            .dest = dest_ptr,
            .src = src_ptr,
            .op = assignment_op
        },
        .tag = nl_ast_expr_assign
    });
}

[[nodiscard]]
cond_entry nl_parse_cond_entry(parser* p, const str* source, bool in_cond_expr) {
    ast_node* check_ptr = nullptr;

    if (!parser_consume_of(p, source, (tk_tag[]) { nl_tk_kw_else }, 1)) {
        check_ptr = nl_parse_logical_or(p, source);
        if (!check_ptr) {
            return nl_invalid_cond_entry_v;
        }
    }

    if (!parser_consume_of(p, source, (tk_tag[]) { nl_tk_colon }, 1)) {
        parser_report_mini_error(p, p->curr.line, p->curr.col, "Expected ':' after conditional case's checking expr.");

        ast_node_del(check_ptr);
        return nl_invalid_cond_entry_v;
    }

    auto body_ptr = nl_parse_next(p, source, in_cond_expr);
    if (!body_ptr) {
        ast_node_del(check_ptr);
        return nl_invalid_cond_entry_v;
    }

    return (cond_entry) {
        .check = check_ptr,
        .body = body_ptr
    };
}

[[nodiscard]]
ast_node* nl_parse_cond(parser* p, const str* source, bool is_expr) {
    parser_consume(p, source); // ? eat prechecked 'cond'

    if (!parser_consume_of(p, source, (tk_tag[]) { nl_tk_lbrace }, 1)) {
        parser_report_mini_error(p, p->curr.line, p->curr.col, "Expected '{' opening a conditional's cases here.");
        return nullptr;
    }

    auto cond_entries = dynbuf_new(sizeof(cond_entry), 4, nl_ast_cond_entry_del);

    auto first_entry = nl_parse_cond_entry(p, source, is_expr);
    if (!first_entry.body) {
        dynbuf_del(&cond_entries);
        return nullptr;
    }
    dynbuf_push(&cond_entries, &first_entry);

    while (!parser_at_eof(p)) {        
        if (!parser_consume_of(p, source, (tk_tag[]) { nl_tk_comma }, 1)) {
            break;
        }

        auto next_entry = nl_parse_cond_entry(p, source, is_expr);
        if (!next_entry.body) {
            dynbuf_del(&cond_entries);
            return nullptr;
        } else if (!next_entry.check) {
            dynbuf_push(&cond_entries, &next_entry);
            break;
        }
    }

    if (!parser_consume_of(p, source, (tk_tag[]) { nl_tk_rbrace }, 1)) {
        parser_report_mini_error(p, p->curr.line, p->curr.col, "Expected '}' closing a conditional's cases here.");
        return nullptr;
    }

    return nl_make_ast_ptr((ast_node) {
        .data.cnd = {
            .entries = cond_entries,
            .is_expr = is_expr
        },
        .tag = nl_ast_cond
    });
}

// ! Syntax:
// ! `<loop> ::= "loop" "(" <init> ";" <check> ";" <update> ")" <block> ("->" <or>)?is_expr`
[[nodiscard]]
ast_node* nl_parse_loop(parser* p, const str* source, bool is_expr) {
    parser_consume(p, source); // ? eat prechecked 'loop'

    if (!parser_consume_of(p, source, (tk_tag[]) { nl_tk_lparen }, 1)) {
        parser_report_mini_error(p, p->curr.line, p->curr.col, "Expected '(' here in this loop.");

        return nullptr;
    }

    auto initial_expr_ptr = nl_parse_next(p, source, true);
    if (!initial_expr_ptr) {
        return nullptr;
    }

    if (!parser_consume_of(p, source, (tk_tag[]) { nl_tk_semicolon }, 1)) {
        parser_report_mini_error(p, p->curr.line, p->curr.col, "Expected ';' after initial loop expression.");

        ast_node_del(initial_expr_ptr);
        return nullptr;
    }

    auto check_expr_ptr = nl_parse_next(p, source, true);
    if (!check_expr_ptr) {
        ast_node_del(initial_expr_ptr);
        return nullptr;
    }

    if (!parser_consume_of(p, source, (tk_tag[]) { nl_tk_semicolon }, 1)) {
        parser_report_mini_error(p, p->curr.line, p->curr.col, "Expected ';' after loop check.");
        ast_node_del(initial_expr_ptr);
        ast_node_del(check_expr_ptr);
        return nullptr;
    }

    auto update_expr_ptr = nl_parse_next(p, source, true);
    if (!update_expr_ptr) {
        ast_node_del(initial_expr_ptr);
        ast_node_del(check_expr_ptr);
        return nullptr;
    }

    if (!parser_consume_of(p, source, (tk_tag[]) { nl_tk_rparen }, 1)) {
        parser_report_mini_error(p, p->curr.line, p->curr.col, "Expected ')' here in this loop.");

        ast_node_del(initial_expr_ptr);
        ast_node_del(check_expr_ptr);
        ast_node_del(update_expr_ptr);
        return nullptr;
    }

    auto loop_body_ptr = nl_parse_block(p, source, is_expr);
    if (!loop_body_ptr) {
        ast_node_del(initial_expr_ptr);
        ast_node_del(check_expr_ptr);
        ast_node_del(update_expr_ptr);
        return nullptr;
    }

    ast_node* result_ptr = nullptr;

    if (is_expr) {
        if (!parser_consume_of(p, source, (tk_tag[]) { nl_tk_arrow }, 1)) {

            parser_report_mini_error(p, p->curr.line, p->curr.col, "Expected '->' before loop expression's result.");
            
            ast_node_del(initial_expr_ptr);
            ast_node_del(check_expr_ptr);
            ast_node_del(update_expr_ptr);
            ast_node_del(loop_body_ptr);
            return nullptr;
        }

        result_ptr = nl_parse_next(p, source, true);

        if (!result_ptr) {
            ast_node_del(initial_expr_ptr);
            ast_node_del(check_expr_ptr);
            ast_node_del(update_expr_ptr);
            ast_node_del(loop_body_ptr);
            return nullptr;
        }
    }

    return nl_make_ast_ptr((ast_node) {
        .data.lp = {
            .init_expr = initial_expr_ptr,
            .check_expr = check_expr_ptr,
            .update_expr = update_expr_ptr,
            .body = loop_body_ptr,
            .result_expr = result_ptr
        },
        .tag = nl_ast_loop
    });
}

[[nodiscard]]
ast_node* nl_parse_loop_abator(parser* p, const str* source, bool is_break) {
    parser_consume(p, source); // ? eat prechecked 'break' / 'continue'

    ast_node* result_expr_ptr = nullptr;

    if (is_break) {
        result_expr_ptr = nl_parse_next(p, source, true);
    }

    if (!parser_consume_of(p, source, (tk_tag[]) { nl_tk_semicolon }, 1)) {
        parser_report_mini_error(p, p->curr.line, p->curr.col, "Expected ';' after this break / continue here.");

        ast_node_del(result_expr_ptr);
        return nullptr;
    }

    return nl_make_ast_ptr((ast_node) {
        .data.lab = {
            .inner = result_expr_ptr
        },
        .tag = nl_ast_loop_abator
    });
}

[[nodiscard]]
ast_node* nl_parse_stmt_ret(parser* p, const str* source) {
    parser_consume(p, source); // ? eat prechecked 'ret'

    auto result_expr_ptr = nl_parse_logical_or(p, source);
    if (!result_expr_ptr) {
        return nullptr;
    }

    if (!parser_consume_of(p, source, (tk_tag[]) { nl_tk_semicolon }, 1)) {
        parser_report_mini_error(p, p->curr.line, p->curr.col, "Expected ';' after this return here.");

        ast_node_del(result_expr_ptr);
        return nullptr;
    }

    return nl_make_ast_ptr((ast_node) {
        .data.sr = {
            .result = result_expr_ptr
        },
        .tag = nl_ast_stmt_ret
    });
}

[[nodiscard]]
def_binding nl_parse_def_binding(parser* p, const str* source) {
    if (!parser_consume_of(p, source, (tk_tag[]) { nl_tk_identifier }, 1)) {
        return nl_invalid_def_bind_v;
    }

    const auto name_tk = p->prev;

    if (!parser_consume_of(p, source, (tk_tag[]) { nl_tk_op_bind }, 1)) {
        return nl_invalid_def_bind_v;
    }

    auto name_init_ptr = nl_parse_next(p, source, true);
    if (!name_init_ptr) {
        return nl_invalid_def_bind_v;
    }

    return (def_binding) {
        .name = name_tk,
        .init = name_init_ptr
    };
}

[[nodiscard]]
ast_node* nl_parse_stmt_def(parser* p, const str* source) {
    parser_consume(p, source); // ? eat prechecked 'def'

    auto var_bindings = dynbuf_new(sizeof(def_binding), 4, nl_ast_def_binding_del);

    auto first_var = nl_parse_def_binding(p, source);
    if (!first_var.init) {
        dynbuf_del(&var_bindings);
        return nullptr;
    }

    dynbuf_push(&var_bindings, &first_var);

    while (!parser_at_eof(p)) {
        if (!parser_consume_of(p, source, (tk_tag[]) { nl_tk_comma }, 1)) {
            break;
        }

        auto next_var = nl_parse_def_binding(p, source);
        if (!next_var.init) {
            dynbuf_del(&var_bindings);
            return nullptr;
        }
        dynbuf_push(&var_bindings, &next_var);
    }

    if (!parser_consume_of(p, source, (tk_tag[]) { nl_tk_semicolon }, 1)) {
        parser_report_mini_error(p, p->curr.line, p->curr.col, "Expected ';' closing variable defines here.");

        dynbuf_del(&var_bindings);
        return nullptr;
    }

    return nl_make_ast_ptr((ast_node) {
        .data.def = {
            .bindings = var_bindings
        },
        .tag = nl_ast_stmt_def
    });
}

[[nodiscard]]
ast_node* nl_parse_block(parser* p, const str* source, bool is_expr) {
    parser_consume(p, source); // ? eat prechecked '{'

    auto block_items = dynbuf_new(sizeof(ast_node), 4, ast_node_del);

    while (!parser_at_eof(p)) {
        if (parser_consume_of(p, source, (tk_tag[]) { nl_tk_rbrace }, 1)) {
            break;
        }

        auto next_item_ptr = nl_parse_next(p, source, false);
        dynbuf_push(&block_items, &next_item_ptr);
    }

    return nl_make_ast_ptr((ast_node) {
        .data.bl = {
            .items = block_items,
            .is_expr = is_expr
        },
        .tag = nl_ast_block
    });
}

[[nodiscard]]
ast_node* nl_parse_expr_stmt(parser* p, const str* source) {
    auto expr_ptr = nl_parse_assignment(p, source);
    if (!expr_ptr) {
        return nullptr;
    }

    if (!parser_consume_of(p, source, (tk_tag[]) { nl_tk_semicolon }, 1)) {
        parser_report_mini_error(p, p->curr.line, p->curr.col, "Expected ';' after expression statement.");

        ast_node_del(expr_ptr);
        return nullptr;
    }

    return nl_make_ast_ptr((ast_node) {
        .data.es = {
            .expr = expr_ptr
        },
        .tag = nl_ast_expr_stmt
    });
}

[[nodiscard]]
ast_node* nl_parse_next(parser* p, const str* source, bool is_expr) {
    const auto next_tk_tag = p->curr.tag;

    if (is_expr) {
        switch (next_tk_tag) {
            case nl_tk_kw_cond: return nl_parse_cond(p, source, is_expr);
            case nl_tk_kw_loop: return nl_parse_loop(p, source, is_expr);
            case nl_tk_kw_fun: return nl_parse_expr_func(p, source);
            default: return nl_parse_assignment(p, source);
        }
    } else {
        switch (next_tk_tag) {
            case nl_tk_kw_def: return nl_parse_stmt_def(p, source);
            case nl_tk_kw_cond: return nl_parse_cond(p, source, is_expr);
            case nl_tk_kw_loop: return nl_parse_loop(p, source, is_expr);
            case nl_tk_kw_break: return nl_parse_loop_abator(p, source, true);
            case nl_tk_kw_continue: return nl_parse_loop_abator(p, source, false);
            case nl_tk_kw_ret: return nl_parse_stmt_ret(p, source);
            case nl_tk_lbrace: return nl_parse_block(p, source, false);
            default: return nl_parse_expr_stmt(p, source);
        }
    }
}



[[nodiscard]]
ast_unit nl_parse_source(const lex_item* special_lexicals, const str* source) {
    auto parse_state = (parser) {
        .tkgen = lexer_new(special_lexicals),
        .prev = { .begin = 0, .n = 1, .line = 1, .col = 1, .tag = nl_tk_unknown },
        .curr = { .begin = 0, .n = 1, .line = 1, .col = 1, .tag = nl_tk_unknown },
        .errors = 0,
        .line = 1,
        .col = 1
    };

    lexer_use_source(&parse_state.tkgen, source);
    parser_consume(&parse_state, source);

    auto temp_roots = dynbuf_new(sizeof(ast_node), 4, ast_node_del);

    while (!parser_at_eof(&parse_state)) {
        auto next_root_ptr = nl_parse_next(&parse_state, source, false);

        if (!next_root_ptr) {
            if (parser_match(&parse_state, (tk_tag[]) { nl_tk_kw_def }, 1)) {
                parser_consume(&parse_state, source);
            }

            while (!parser_at_eof(&parse_state)) {
                if (parse_state.curr.tag == nl_tk_kw_def) {
                    break;
                }

                parser_consume(&parse_state, source);
            }
        } else {   
            dynbuf_push(&temp_roots, next_root_ptr);
        }
    }

    if (parse_state.errors > 0) {
        dynbuf_del(&temp_roots);

        return (ast_unit) {
            .roots = dynbuf_dud(),
            .source = str_dud()
        };
    } else {   
        return (ast_unit) {
            .roots = temp_roots,
            .source = *source
        };
    }
}
