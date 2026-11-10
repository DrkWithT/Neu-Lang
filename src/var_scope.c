#include "var_scope.h"



void scope_del(void *s) {
    auto scope_self = (scope*)s;

    dynbuf_del(&scope_self->vars);
}
