#include "token.h"

void tk_output(FILE* os, const tk* token) {
    fprintf(os, "Token(begin = %d, length = %d, line = %d, col = %d, ok = %s)", 
        token->begin,
        token->n,
        token->line,
        token->col,
        (token->tag != nl_tk_unknown) ? "T" : "F"
    );
}