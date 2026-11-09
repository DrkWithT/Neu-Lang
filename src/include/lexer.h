#pragma once

#include "token.h"



typedef struct nl_lex_item_t {
    str s;
    tk_tag tag;
} lex_item;

typedef struct nl_lexer_t {
    const lex_item* specials;
    uint16_t pos;
    uint16_t end;
    uint16_t line;
    uint16_t col;
} lexer;

[[nodiscard]]
lexer lexer_new(const lex_item* lexical_specials);

void lexer_use_source(lexer* self, const str* source);

[[nodiscard]]
tk lexer_next(lexer* self, const str* source);
