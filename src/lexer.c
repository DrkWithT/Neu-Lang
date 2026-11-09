#include "lexer.h"



static const str neu_space_group = {
    .s = " \n\t\r\v",
    .n = 5
};

static const str neu_ascii_ws_escape_group = {
    .s = "ntrv",
    .n = 4
};

static const str neu_op_group = {
    .s = "?%*/+-=<>!&|",
    .n = 13
};

typedef bool(*nl_lexical_fn)(char c);



[[nodiscard]]
static inline bool check_digit(char c) {
    return c >= '0' && c <= '9';
}

[[nodiscard]]
static inline bool check_alpha(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

[[nodiscard]]
static inline bool check_hex_digit(char c) {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F');
}

[[nodiscard]]
static inline bool check_group(char c, const str* c_group) {
    for (size_t i = 0; i < c_group->n; i++) {
        if (c_group->s[i] == c) return true;
    }

    return false;
}

[[nodiscard]]
static inline bool check_spaces(char c) {
    return check_group(c, &neu_space_group);
}

[[nodiscard]]
static inline bool check_like_op(char c) {
    return check_group(c, &neu_op_group);
}

[[nodiscard]]
static inline bool check_alphanum(char c) {
    return check_alpha(c) || check_digit(c);
}



[[nodiscard]]
bool lexer_at_eos(const lexer* self) {
    return self->pos >= self->end;
}

[[nodiscard]]
char lexer_peek_off(lexer* self, const str* source, uint16_t offset) {
    const uint16_t peek_pos = self->pos + offset;

    if (peek_pos < self->end) {
        return source->s[peek_pos];
    }

    return '\0';
}

void lexer_consume(lexer* self, const str* source) {
    const auto temp_c = source->s[self->pos];

    if (temp_c == '\n') {
        self->line++;
        self->col = 1;
    } else {
        self->col++;
    }

    self->pos++;
}

[[nodiscard]]
tk_tag lexer_find_special(lexer* self, const str* source, tk_tag fallback_tag) {
    for (auto specials_it = self->specials; !str_empty(&specials_it->s); specials_it++) {
        if (str_eq_str(&specials_it->s, source)) return specials_it->tag;
    }

    return fallback_tag;
}



[[nodiscard]]
tk lexer_lex_single(lexer* self, const str* source, tk_tag tag) {
    const auto tk_begin = self->pos;
    const auto tk_line = self->line;
    const auto tk_col = self->col;

    lexer_consume(self, source);

    return mk_token(tk_begin, self->pos - tk_begin, tk_line, tk_col, tag);
}

[[nodiscard]]
tk lexer_lex_between(lexer* self, const str* source, char delim, char forbidden_char, tk_tag tag) {
    lexer_consume(self, source);

    const auto tk_begin = self->pos;
    const auto tk_line = self->line;
    const auto tk_col = self->col;
    auto tk_valid = false;

    while (!lexer_at_eos(self)) {
        const auto c = lexer_peek_off(self, source, 0);

        if (c == delim) {
            lexer_consume(self, source);
            tk_valid = true;
            break;
        } else {
            lexer_consume(self, source);
        }
    }

    return mk_token(tk_begin, self->pos - tk_begin - 1, tk_line, tk_col, tag);
}

[[nodiscard]]
tk lexer_lex_while(lexer* self, const str* source, nl_lexical_fn f, tk_tag tag) {
    const auto tk_begin = self->pos;
    const auto tk_line = self->line;
    const auto tk_col = self->col;

    while (!lexer_at_eos(self)) {
        const auto c = lexer_peek_off(self, source, 0);

        if (f(c)) {
            lexer_consume(self, source);
        } else {
            break;
        }
    }

    return mk_token(tk_begin, self->pos - tk_begin, tk_line, tk_col, tag);
}

[[nodiscard]]
tk lexer_lex_number(lexer* self, const str* source) {
    const auto tk_begin = self->pos;
    const auto tk_line = self->line;
    const auto tk_col = self->col;

    while (!lexer_at_eos(self)) {
        const auto c = lexer_peek_off(self, source, 0);

        if (check_digit(c)) {
            lexer_consume(self, source);
        } else {
            break;
        }
    }

    const auto maybe_dot = lexer_peek_off(self, source, 0);

    if (maybe_dot != '.') {
        return mk_token(tk_begin, self->pos - tk_begin, tk_line, tk_col, nl_tk_lt_int);
    } else {
        lexer_consume(self, source);
    }

    while (!lexer_at_eos(self)) {
        const auto c = lexer_peek_off(self, source, 0);

        if (check_digit(c)) {
            lexer_consume(self, source);
        } else {
            break;
        }
    }

    return mk_token(tk_begin, self->pos - tk_begin, tk_line, tk_col, nl_tk_lt_num);
}

[[nodiscard]]
bool lexer_scan_esc(lexer* self, const str* source) {
    lexer_consume(self, source); // ? eat prechecked '\\'

    const auto peek_1 = lexer_peek_off(self, source, 0);

    if (check_group(peek_1, &neu_ascii_ws_escape_group)) {
        lexer_consume(self, source);
        return true;
    } else if (peek_1 != 'x') {
        return false;
    } else {   
        lexer_consume(self, source);
    }

    const auto peek_2 = lexer_peek_off(self, source, 1);

    if (!check_hex_digit(peek_2)) {
        return false;
    } else {
        lexer_consume(self, source);
    }

    const auto peek_3 = lexer_peek_off(self, source, 1);

    if (!check_hex_digit(peek_3)) {
        return false;
    } else {
        lexer_consume(self, source);
    }

    return true;
}

[[nodiscard]]
tk lexer_lex_string(lexer* self, const str* source, char delim) {
    lexer_consume(self, source); // ? eat prechecked delimiter quote

    const auto tk_begin = self->pos;
    const auto tk_line = self->line;
    const auto tk_col = self->col;
    auto is_closed = false;
    auto is_escaped = false;
    auto is_validated = true;

    while (!lexer_at_eos(self)) {
        const auto c = lexer_peek_off(self, source, 0);

        if (c == '\n') {
            break;
        } else if (c == delim) {
            lexer_consume(self, source);
            is_closed = true;
            break;
        } else if (c == '\\') {
            is_escaped = true;

            if (!lexer_scan_esc(self, source)) {
                is_validated = false;
                break;
            }
        } else {
            lexer_consume(self, source);
        }
    }

    tk_tag temp_tag;

    if (!is_closed || !is_validated) {
        temp_tag = nl_tk_unknown;
    } else if (is_escaped) {
        temp_tag = nl_tk_lt_esc_str;
    } else {
        temp_tag = nl_tk_lt_plain_str;
    }

    return mk_token(tk_begin, self->pos - tk_begin - 1, tk_line, tk_col, temp_tag);
}

[[nodiscard]]
tk lexer_lex_symbolic(lexer* self, const str* source) {
    auto temp_tk = lexer_lex_while(self, source, check_like_op, nl_tk_unknown);
    const auto temp_lexeme = tk_get_lexeme(&temp_tk, source);

    temp_tk.tag = lexer_find_special(self, &temp_lexeme, nl_tk_unknown);

    return temp_tk;
}

[[nodiscard]]
tk lexer_lex_word(lexer* self, const str* source) {
    auto temp_tk = lexer_lex_while(self, source, check_alphanum, nl_tk_identifier);
    const auto temp_lexeme = tk_get_lexeme(&temp_tk, source);

    temp_tk.tag = lexer_find_special(self, &temp_lexeme, nl_tk_identifier);

    return temp_tk;
}


[[nodiscard]]
lexer lexer_new(const lex_item* lexical_specials) {
    return (lexer) {
        .specials = lexical_specials,
        .pos = 0,
        .end = 0,
        .line = 1,
        .col = 1
    };
}

void lexer_use_source(lexer* self, const str* source) {
    self->pos = 0;
    self->end = source->n;
    self->line = 1;
    self->col = 1;
}

[[nodiscard]]
tk lexer_next(lexer* self, const str* source) {
    if (lexer_at_eos(self)) {
        return mk_token(self->end, 1, self->line, self->col + 1, nl_tk_eof);
    }

    const auto peek_1 = lexer_peek_off(self, source, 0);

    switch (peek_1) {
        case ',': return lexer_lex_single(self, source, nl_tk_comma);
        case ':': return lexer_lex_single(self, source, nl_tk_colon);
        case ';': return lexer_lex_single(self, source, nl_tk_semicolon);
        case '(': return lexer_lex_single(self, source, nl_tk_lparen);
        case ')': return lexer_lex_single(self, source, nl_tk_rparen);
        case '{': return lexer_lex_single(self, source, nl_tk_lbrace);
        case '}': return lexer_lex_single(self, source, nl_tk_rbrace);
        case '[': return lexer_lex_single(self, source, nl_tk_lbrack);
        case ']': return lexer_lex_single(self, source, nl_tk_rbrack);
        case '`': return lexer_lex_between(self, source, peek_1, '\0', nl_tk_comment);
        case '\"': case '\'': return lexer_lex_string(self, source, peek_1);
        default: break;
    }

    if (check_spaces(peek_1)) {
        return lexer_lex_while(self, source, check_spaces, nl_tk_spaces);
    } else if (check_digit(peek_1)) {
        return lexer_lex_number(self, source);
    } else if (check_like_op(peek_1)) {
        return lexer_lex_symbolic(self, source);
    } else if (check_alpha(peek_1)) {
        return lexer_lex_word(self, source);
    } else {
        return lexer_lex_single(self, source, nl_tk_unknown);
    }
}
