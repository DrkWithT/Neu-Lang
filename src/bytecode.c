#include "bytecode.h"



void chunk_del(void *c) {
    auto chunk_self = (chunk*)c;

    free(chunk_self->icv);
    chunk_self->icv = nullptr;
    free(chunk_self->kd);
    chunk_self->kd = nullptr;
    free(chunk_self->bc);
    chunk_self->bc = nullptr;
}

void program_del(void* pg) {
    auto program_self = (program*)pg;

    ent_pool_del(&program_self->pool_c);
    ent_pool_del(&program_self->pool_o);
    ent_pool_del(&program_self->pool_s);
    dynbuf_del(&program_self->chunks);
}
