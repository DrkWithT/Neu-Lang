#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct nl_string_t string;

/// ASCII string view
typedef struct nl_str_t {
    const char* s;
    size_t n;
} str;

[[nodiscard]]
str str_dud();

[[nodiscard]]
str str_new(const char* base, size_t len);

[[nodiscard]]
str str_from_cstr(const char* cstr);

[[nodiscard]]
str str_from_string(const string* s);

[[deprecated("This is a no-op function because str does not own its data.")]]
static inline void str_del([[maybe_unused]] str* self) {}

[[nodiscard]]
static inline bool str_empty(const str* self) {
    return self->n == 0 || !self->s;
}

[[nodiscard]]
static inline size_t str_size(const str* self) {
    return self->n;
}

[[nodiscard]]
bool str_eq_raw(const str* self, const char* cstr, size_t n);

[[nodiscard]]
bool str_eq_str(const str* self, const str* rhs);

void str_output(FILE* fs, const str* s);



/// owning, ASCII string
typedef struct nl_string_t {
    char* data;
    size_t n;
    size_t cap;
} string;

[[nodiscard]]
string string_dud();

[[nodiscard]]
string string_new(const char* cstr, size_t initial_len);

[[nodiscard]]
string string_from_str(const str* sv);

void string_del(void* self);

[[nodiscard]]
static inline bool string_empty(const string* self) {
    return !self->data || self->n == 0;
}

[[nodiscard]]
static inline size_t string_size(const string* self) {
    return self->n;
}

[[nodiscard]]
static inline const char* string_data(const string* self) {
    return self->data;
}

[[nodiscard]]
static inline char* string_data_mut(string* self) {
    return self->data;
}

[[nodiscard]]
bool string_eq_str(const string* self, const str* s);

[[nodiscard]]
bool string_eq_string(const string* self, const string* rhs);

void string_append_raw(string* self, const char* s, size_t n);

void string_append_str(string* self, const str* rhs);

void string_output(FILE* fs, const string* s);
