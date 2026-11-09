#include "dynbuf.h"


static constexpr dynbuf nl_dynbuf_dud_v = {
    .data = nullptr,
    .item_size = 0,
    .cap = 0,
    .n = 0
};

[[nodiscard]]
dynbuf dynbuf_dud() {
    return nl_dynbuf_dud_v;
}

[[nodiscard]]
dynbuf dynbuf_new(size_t item_size, uint32_t init_cap, nl_deleter_t del_fn) {
    const size_t allocation_sz = init_cap * item_size;
    NL_ALLOC_N(allocation_sz, uint8_t, temp_buffer, __FILE__, __LINE__)

    return (dynbuf) {
        .data = temp_buffer,
        .del_fn = del_fn,
        .item_size = item_size,
        .cap = init_cap,
        .n = 0
    };
}

void dynbuf_del(dynbuf* self) {
    if (!self->data) {
        return;
    }

    auto buffer_data = self->data;
    const auto deleter_func = self->del_fn;
    const auto item_sz = self->item_size;
    const auto current_length = self->n;

    if (deleter_func != nullptr) {
        for (uint32_t del_index = 0; del_index < current_length; del_index++) {
            deleter_func(buffer_data + del_index * item_sz);
        }
    }

    free(self->data);
    self->data = nullptr;
}

static void dynbuf_grow_to(dynbuf* self, size_t new_buffer_cap) {
    auto next_buffer = realloc(self->data, self->item_size * new_buffer_cap);

    if (!next_buffer) {
        NL_PANIC("Failed to grow dynbuf's data here.", __FILE__, __LINE__)
    }

    self->data = next_buffer;
    self->cap = new_buffer_cap;
}

void* dynbuf_push(dynbuf* self, const void* src) {
    const auto next_len = self->n + 1;

    if (next_len >= self->cap) {
        dynbuf_grow_to(self, (3 * self->cap) / 2);
    }

    auto dest_ptr = self->data + self->n * self->item_size;

    memcpy(dest_ptr, src, self->item_size);
    self->n = next_len;

    return dynbuf_last_mut(self);
}

[[maybe_unused]]
size_t dynbuf_pop(dynbuf* self) {
    // ! IMPORTANT: this does lazy deletion to avoid extra reallocations / shrinking.
    self->del_fn(dynbuf_last_mut(self));
    
    const auto next_len = --self->n;

    return next_len;
}
