#include "basics.h"
#include "strs.h"



static constexpr str dud_str_v = {
    .s = nullptr,
    .n = 0
};

static constexpr size_t defaulted_str_length = 16;

static constexpr size_t maximal_str_length = 1024;



[[nodiscard]]
str str_dud() {
    return dud_str_v;
}

[[nodiscard]]
str str_new(const char* base, size_t len) {
    return (str) {
        .s = base,
        .n = len
    };
}



[[nodiscard]]
str str_from_cstr(const char* cstr) {
    return (str) {
        .s = cstr,
        .n = strnlen(cstr, maximal_str_length)
    };
}

[[nodiscard]]
str str_from_string(const string* s) {
    return (str) {
        .s = s->data,
        .n = s->n
    };
}

[[nodiscard]]
bool str_eq_raw(const str* self, const char* cstr, size_t n) {
    if (!self->s && !cstr) {
        return true;
    } else if (!self->s || !cstr) {
        return false;
    } else if (self->n != n) {
        return false;
    }

    return !strncmp(self->s, cstr, n);
}

[[nodiscard]]
bool str_eq_str(const str* self, const str* rhs) {
    if (str_empty(self) && str_empty(rhs)) {
        return true;
    } else if (str_empty(self) || str_empty(rhs)) {
        return false;
    } else if (self->n != rhs->n) {
        return false;
    }

    return !strncmp(self->s, rhs->s, self->n);
}

void str_write_to(FILE* fs, const str* s) {
    fwrite(s->s, sizeof(char), s->n, fs);
}



[[nodiscard]]
string string_dud() {
    constexpr auto initial_capacity = defaulted_str_length;

    NL_ALLOC_N(initial_capacity, char, temp_def_buf, __FILE__, __LINE__)

    return (string) {
        .data = temp_def_buf,
        .n = 0,
        .cap = initial_capacity,
    };
}

// ! API NOTE: `initial_len` MUST not be longer than the contiguous char sequence pointed to be cstr, even as a C-string literal.
[[nodiscard]]
string string_new(const char* cstr, size_t initial_len) {
    const auto initial_capacity = (3 * initial_len) / 2;

    // ? NOTE: This would "panic" the program otherwise if temp_buf could not be allocated from calloc internally. After this, we can be sure `temp_buf` exists to copy N chars into via `cstr`.
    NL_ALLOC_N(initial_capacity, char, temp_buf, __FILE__, __LINE__)

    strncpy(temp_buf, cstr, initial_len);

    return (string) {
        .data = temp_buf,
        .n = initial_len,
        .cap = initial_capacity
    };
}

[[nodiscard]]
string string_from_str(const str* sv) {
    return string_new(sv->s, sv->n);
}

void string_del(void* self) {
    auto str_self = (string*)self;

    if (!str_self->data) {
        return;
    }

    free(str_self->data);
    str_self->data = nullptr;
}

[[nodiscard]]
bool string_eq_str(const string* self, const str* s) {
    auto view_self = str_from_string(self);

    return str_eq_str(&view_self, s);
}

[[nodiscard]]
bool string_eq_string(const string* self, const string* rhs) {
    const auto self_view = str_from_string(self);
    const auto rhs_view = str_from_string(rhs);

    return str_eq_str(&self_view, &rhs_view);
}

static void string_grow_to(string* self, size_t next_capacity) {
    char* next_buf = realloc(self->data, sizeof(char) * next_capacity);

    if (!next_buf) {
        NL_PANIC("Failed to re-allocate string data here.", __FILE__, __LINE__)
    }

    self->data = next_buf;
    self->cap = next_capacity;
}

void string_append_raw(string* self, const char* s, size_t n) {
    const size_t next_len = self->n + n;
    const size_t old_capacity = self->cap;

    if (next_len >= old_capacity) {
        string_grow_to(self, (3 * next_len) / 2);
    }

    strncpy(self->data + self->n, s, n);
    self->n = next_len;
}

void string_append_str(string* self, const str* rhs) {
    string_append_raw(self, rhs->s, rhs->n);
}

void string_output(FILE* fs, const string* s) {
    fwrite(s->data, sizeof(char), s->n, fs);
}
