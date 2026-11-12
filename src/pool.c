#include "pool.h"



static constexpr int32_t nl_invalid_ent_pool_id = -1;

static constexpr entity_handle nl_invalid_ent_handle = {
    .p = nullptr,
    .id = nl_invalid_ent_pool_id
};



[[nodiscard]]
static inline size_t ent_pool_calc_gc_cost(size_t object_size, int32_t max_id, int32_t ratio_top, int32_t ratio_bottom) {
    return (object_size * max_id * ratio_top) / ratio_bottom;
}

[[nodiscard]]
static int32_t ent_pool_gen_id(ent_pool* p) {
    auto maybe_free_id_ptr = (int32_t*)dynbuf_last(&p->free_ids);
    if (!maybe_free_id_ptr) {
        const auto fresh_id = p->next_id;

        if (fresh_id >= p->max_id) {
            NL_PANIC("Exhausted ent_pool here: the fresh_id has exceeded the max_id.", __FILE__, __LINE__)
        } else {
            p->next_id++;
        }

        return fresh_id;
    } else {
        const auto reused_id = *maybe_free_id_ptr;

        dynbuf_pop(&p->free_ids);

        return reused_id;
    }
}

[[nodiscard]]
ent_pool ent_pool_new(size_t object_size, int32_t capacity, nl_deleter_t df) {
    if (!df) {
        NL_PANIC("An invalid 'df' was passed to ent_pool_new() here: a non-null deleter is required for objects!", __FILE__, __LINE__);
    }

    if (capacity < 0) {
        NL_PANIC("Invalid capacity passed to ent_pool_new() here: expected positive count!", __FILE__, __LINE__)
    }

    auto temp_mem = dynbuf_new(object_size, capacity, df);
    auto temp_free_ids = dynbuf_new(sizeof(int32_t), capacity, nullptr);

    return (ent_pool) {
        .mem = temp_mem,
        .free_ids = temp_free_ids,
        .deleter = df,
        .cost = 0,
        .gc_cost = ent_pool_calc_gc_cost(object_size, capacity, 2, 3),  // ! GC collect this pool if ~ 67% is full.
        .next_id = 0,
        .max_id = capacity,
        .tenure_id = 0
    };
}

void ent_pool_del(ent_pool* self) {
    dynbuf_del(&self->mem);
    dynbuf_del(&self->free_ids);
}

[[nodiscard]]
entity_handle_mut ent_pool_add(ent_pool* self, const void* object_p) {
    auto dest_id = ent_pool_gen_id(self);
    auto dest_cell = dynbuf_get_mut(&self->mem, dest_id);

    memcpy(dest_cell, object_p, self->obj_size);

    return (entity_handle_mut) {
        .p = dest_cell,
        .id = dest_id
    };
}

void ent_pool_erase(ent_pool* self, int32_t cell_id) {
    if (cell_id <= self->tenure_id || cell_id >= self->max_id) {
        NL_PANIC("Expected ID between [0; max_id) passed to ent_pool_erase()!", __FILE__, __LINE__)
    }

    self->deleter(dynbuf_get_mut(&self->mem, cell_id));
    dynbuf_push(&self->free_ids, &cell_id);
}

void ent_pool_erase_unchecked(ent_pool* self, int32_t cell_id) {
    self->deleter(dynbuf_get_mut(&self->mem, cell_id));
    dynbuf_push(&self->free_ids, &cell_id);
}
