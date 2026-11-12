#pragma once

#include "dynbuf.h"



/**
 ### ABOUT
 - Represents a "locator" into a pool of objects. The `p` pointer points to the actual entity AND the `id` is not `-1` iff the entity exists.
 ### USES
 - If the interpreter needs to register some built-in objects in Neu, a handle can be returned to access the object and patch some data into it.
 - The `id` can be stored in a compact VM `Value` to refer to objects.
 */
typedef struct nl_entity_handle_mut_t {
    void* p;        // non-owning mutable pointer to object
    int32_t id;     // ID into object's pool
} entity_handle_mut;

/// Readonly variation of `entity_handle_mut`.
typedef struct nl_entity_handle_t {
    const void* p;
    int32_t id;
} entity_handle;

/**
 ### ABOUT
 - Contains a buffer of raw memory containing one type of object. The memory itself just stores the bits of the objects contiguously, but is still raw bytes from malloc. IDs that each correspond to the location of an object (address of 1st byte) are used for getting/setting objects.
 - See comments in this type for a nice ASCII diagram ;)
 */
typedef struct nl_entity_pool_t {
    /// Mem: | ... object-0-bits ... | ... object-1-bits ... | ... object-2-bits ... | ... |
    /// IDs: 0                       1                       2                         ... N
    dynbuf mem;         // stores memory with object bits in-place
    dynbuf free_ids;    // stores int32_t values in-place
    nl_deleter_t deleter;
    size_t obj_size;    // size in bytes of type
    size_t cost;        // current "score" of pooled memory usage
    size_t gc_cost;     // "score" of memory usage where GC sweep is needed
    int32_t next_id;    // next ID (corresponds to `mem.data + sizeof(entity_type) * ID`)
    int32_t max_id;     // maximum usable ID
    int32_t tenure_id;
} ent_pool;

[[nodiscard]]
ent_pool ent_pool_new(size_t object_size, int32_t capacity, nl_deleter_t df);
void ent_pool_del(ent_pool* self);

static inline void ent_pool_tenure(ent_pool* p) {
    p->tenure_id = p->next_id - 1;
}

[[nodiscard]]
static inline bool ent_pool_gc_ripe(const ent_pool* p) {
    return p->cost >= p->gc_cost;
}

[[nodiscard]]
static inline void* ent_pool_data_mut(ent_pool* self) {
    return self->mem.data;
}

[[nodiscard]]
static inline const void* ent_pool_data(ent_pool* self) {
    return self->mem.data;
}

[[nodiscard]]
static inline int32_t ent_pool_max_id(const ent_pool* self) {
    return self->max_id;
}

[[nodiscard]]
static inline entity_handle ent_pool_get(const ent_pool* self, int32_t id) {
    if (id < 0 || id >= self->max_id) {
        NL_PANIC("Invalid ID passed to immutable ent_pool_get(): expected id in [0; max_id)", __FILE__, __LINE__)
    }

    return (entity_handle) {
        .p = dynbuf_get(&self->mem, id),
        .id = id
    };
}

[[nodiscard]]
static inline entity_handle_mut ent_pool_get_mut(ent_pool* self, int32_t id) {
    if (id < 0 || id >= self->max_id) {
        NL_PANIC("Invalid ID passed to mutating ent_pool_get_mut(): expected id in [0; max_id)", __FILE__, __LINE__)
    }

    return (entity_handle_mut) {
        .p = dynbuf_get_mut(&self->mem, id),
        .id = id
    };
}

[[nodiscard]]
static inline entity_handle ent_pool_get_unchecked(const ent_pool* self, int32_t id) {
    return (entity_handle) {
        .p = dynbuf_get(&self->mem, id),
        .id = id
    };
}

[[nodiscard]]
static inline entity_handle_mut ent_pool_get_mut_unchecked(ent_pool* self, int32_t id) {
    return (entity_handle_mut) {
        .p = dynbuf_get_mut(&self->mem, id),
        .id = id
    };
}

[[nodiscard]]
entity_handle_mut ent_pool_add(ent_pool* self, const void* object_p);

void ent_pool_erase(ent_pool* self, int32_t cell_id);

void ent_pool_erase_unchecked(ent_pool* self, int32_t cell_id);
