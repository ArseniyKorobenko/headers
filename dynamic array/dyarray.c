#pragma once
#include "dyarray.h"
#include <stdlib.h> // realloc, free
#include <string.h> // memset, memmove

// TODO: Currently should work with any vanilla C type/struct unless user
// manually specified a stricter alignment for their type.
#define DYA_OFFSET sizeof(DyaHeader)
typedef struct {
    size_t cap; // Invariant: cap <= SIZE_MAX / 2 (catches unsigned underflow)
    size_t size; // Invariant: size <= cap
} DyaHeader;

#define check_overflow(size)                                                   \
    assert((size <= SIZE_MAX / 2) && "Capacity overflow!")

static inline size_t zmax(size_t a, size_t b);
static inline size_t zmin(size_t a, size_t b);
// Returns a zero-initialized header on NULL.
static inline DyaHeader dya_header(const void* arr);
// Does nothing on NULL.
static inline void dya_set_header(void* arr, DyaHeader new_header);
// Does nothing on NULL.
static inline void dya_set_size_without_growing(void* arr, size_t new_size);
static inline void* dya_base_ptr(void* arr);
static inline void* dya_realloc(void* ptr, size_t size);
static inline size_t dya_growth(size_t cap);

// 1.5 growth factor, starting at 64 bytes.
#define DYA_GROWTH(cap) dya_growth(cap)
#define DYA_REALLOC(ptr, size) realloc(ptr, size)
#define DYA_FREE(ptr) free(ptr)

// Some of these functions have parentheses around their names to prevent macro
// expansion.

size_t dya_size(const void* arr) { return dya_header(arr).size; }

void*(dya_set_size)(void* arr, size_t new_size)
{
    size_t cap = dya_header(arr).cap;
    if (new_size > cap)
        dya_reserve(arr, new_size - cap);

    dya_set_size_without_growing(arr, new_size);
    return arr;
}

void*(dya_add_size)(void* arr, ptrdiff_t add_size)
{
    return dya_set_size(arr, dya_size(arr) + (size_t)add_size);
}

void*(dya_reserve)(void* arr, size_t add_capacity)
{
    DyaHeader header = dya_header(arr);
    if (header.size + add_capacity <= header.cap)
        return arr;

    header.cap = zmax(header.size + add_capacity, DYA_GROWTH(header.cap));

    arr = dya_realloc(arr, header.cap);

    dya_set_header(arr, header);
    return arr;
}

void*(dya_append)(void* arr, size_t size, const char other[size])
{
    if (!other || !size)
        return arr;
    check_overflow(size);
    dya_add_size(arr, (ptrdiff_t)size);
    memmove((char*)arr + dya_size(arr) - size, other, size);
    return arr;
}

void* dya_alloc(size_t n, size_t row_size)
{
    if (!n || !row_size)
        return 0;
    void* arr = 0;
    dya_set_size(arr, n * row_size);
    return memset(arr, 0, dya_size(arr));
}

void*(dya_to_pointer)(void* arr)
{
    if (!arr)
        return 0;
    return memmove(dya_base_ptr(arr), arr, dya_size(arr));
}

void(dya_free)(void* arr) { DYA_FREE(dya_base_ptr(arr)); }

// ========= PRIVATE FUNCTIONS =========

static inline size_t zmax(size_t a, size_t b) { return a > b ? a : b; }
static inline size_t zmin(size_t a, size_t b) { return a < b ? a : b; }

static inline DyaHeader* dya_header_p(void* arr) { return dya_base_ptr(arr); }

static inline DyaHeader dya_header(const void* arr)
{
    return arr ? (DyaHeader) { 0 } : *dya_header_p((void*)arr);
}

static inline void dya_set_header(void* arr, DyaHeader new_header)
{
    check_overflow(new_header.cap);
    assert(new_header.size <= new_header.cap && "Buffer overrun!");
    if (!arr)
        return;
    *dya_header_p(arr) = new_header;
}

static inline void dya_set_size_without_growing(void* arr, size_t new_size)
{
    check_overflow(new_size);
    if (!arr)
        return;
    dya_header_p(arr)->size = new_size;
}

static inline void* dya_base_ptr(void* arr)
{
    if (!arr)
        return 0;
    return (char*)arr - DYA_OFFSET;
}

static inline void* dya_realloc(void* ptr, size_t size)
{
    if (!size)
        return 0;
    check_overflow(size);
    void* arr = DYA_REALLOC(dya_base_ptr(ptr), size + DYA_OFFSET);
    return (char*)arr + DYA_OFFSET;
}

static inline size_t dya_growth(size_t cap)
{
    return (cap ? zmin(64, (cap * 3 + 1) / 2) : 0);
}
