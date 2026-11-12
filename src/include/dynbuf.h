#pragma once

#include "basics.h"

typedef struct nl_dynbuf_t {
    void* data;
    nl_deleter_t del_fn;
    size_t item_size;
    uint32_t n;
    uint32_t cap;
} dynbuf;

[[nodiscard]]
dynbuf dynbuf_dud();

[[nodiscard]]
dynbuf dynbuf_new(size_t item_size, uint32_t init_cap, nl_deleter_t deleter);

void dynbuf_del(dynbuf* self);

[[nodiscard]]
static inline const void* dynbuf_data(const dynbuf* self) {
    return self->data;
}

[[nodiscard]]
static inline bool dynbuf_empty(const dynbuf* self) {
    return !self->data || self->n == 0;
}

[[nodiscard]]
static inline size_t dynbuf_size(const dynbuf* self) {
    return self->n;
}

void* dynbuf_push(dynbuf* self, const void* src);

[[maybe_unused]]
size_t dynbuf_pop(dynbuf* self);

[[nodiscard]]
static inline const void* dynbuf_last(const dynbuf* self) {
    if (dynbuf_empty(self)) {
        return nullptr;
    }

    return self->data + (self->n - 1) * self->item_size;
}

[[nodiscard]]
static inline void* dynbuf_last_mut(dynbuf* self) {
    if (dynbuf_empty(self)) {
        return nullptr;
    }

    return self->data + (self->n - 1) * self->item_size;
}

[[nodiscard]]
static inline const void* dynbuf_get(const dynbuf* self, uint32_t pos) {
    if (pos >= self->n) {
        return nullptr;
    }

    return self->data + pos * self->item_size;
}

[[nodiscard]]
static inline void* dynbuf_get_mut(dynbuf* self, uint32_t pos) {
    if (pos >= self->n) {
        return nullptr;
    }

    return self->data + pos * self->item_size;
}
