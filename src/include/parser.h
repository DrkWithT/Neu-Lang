#pragma once

#include "lexer.h"
#include "ast.h"



typedef struct nl_parser_t {
    lexer tkgen;
    tk prev;
    tk curr;
    uint32_t errors;
    uint16_t line;
    uint16_t col;
} parser;

[[nodiscard]]
ast_unit nl_parse_source(const lex_item* special_lexicals, const str* source);
