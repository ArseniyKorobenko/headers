/* ========= DYA.H - DYNAMIC ARRAYS FOR C ========= */
/*
 * This is a header-only C library.
 * To use it, ONE of the files that include it must
 * #define DYA_IMPLEMENTATION before #including this file.
 *
 * Usage example:
 *     // Create a 40-wide array of struct tm
 *
 *     struct tm* timestamps = dya_alloc(40, sizeof(*timestamps));
 *     dya_fill(timestamps, struct tm, .tm_year = 100, .tm_mon = 11 );
 *
 *     dya_push(timestamps, (struct tm) { .tm_year = 101, .tm_mon = 7 });
 *     dya_push(timestamps, (struct tm) { .tm_year = 123, .tm_mon = 4 });
 *
 *     struct tm* timestamps_copy = 0;
 *     dya_append(timestamps_copy, 5 * sizeof(*timestamps), timestamps);
 *     dya_append(timestamps_copy, dya_size(timestamps), timestamps);
 *
 *     // Create a 5*10*20 int array of 42s
 *
 *     int(*arr)[10][20] = dya_alloc(5, sizeof(*arr));
 *     dya_fill(arr, int, 42);
 *
 *     arr[2][8][16] += timestamps[10].tm_year;
 *
 *     dya_free(timestamps_copy);
 *     dya_free(timestamps);
 *     dya_free(arr);
 *
 * The dynamic array behaves just like a pointer, except it also
 * has size and capacity fields beside it.
 * You may access these fields using dya_size() and dya_cap().
 *
 * Because the dynamic array may be reallocated, you must always
 * reassign it to the return value of the function.
 *
 * The listed functions may only be used on dynamic arrays,
 * with the exception of dya_append(),
 * which can take a regular pointer and append/convert it to a dynamic array.
 *
 * Dynamic arrays may only be freed with dya_free(), so if you want to
 * pass it to a function that doesn't know about that, convert it with
 * dya_to_pointer().
 *
 * If you wish to use your own memory allocator, you may
 * #define DYA_REALLOC and DYA_FREE in the same file that
 * defines DYA_IMPLEMENTATION.
 */

// I don't know which linkage is correct tbh. You can redefine it if you do.
#ifndef DYA_LINKAGE
#define DYA_LINKAGE extern inline
#endif

#ifndef INCLUDE_DYA_H
#define INCLUDE_DYA_H

#include "macro_var.h"
#include <stddef.h> // for size_t

/* ========= FUNCTION INTERFACE ========= */

// Length of the array in rows.
#define dya_len(arr) (dya_size(arr) / sizeof(*arr))
// Length of the array in bytes.
DYA_LINKAGE size_t dya_size(const void* arr);
DYA_LINKAGE void dya_set_size(void* arr, size_t size);
// Maximum array capacity.
DYA_LINKAGE size_t dya_cap(const void* arr);
DYA_LINKAGE void dya_set_cap(void* arr, size_t len);

// Allocate a new array filled with 0.
#define dya_init(n, row_size) dya_alloc(n, row_size)
// Allocate a new array filled with 0.
DYA_LINKAGE void* dya_alloc(size_t n, size_t row_size);

/* ========= FUNCTIONS THAT ARE REDEFINED AS MACROS ========= */
// These macros reassign `arr` to the return value of the function.

// Reserve capacity for at least `add_size` additional elements.
// May reserve more space than requested to prevent frequent reallocations.
// Does nothing if capacity is already high enough.
// `arr` may be NULL, in which case a fresh array of `add_size` will be created.
DYA_LINKAGE void* dya_reserve(void* arr, size_t add_size);

// Copies `size` bytes from `other` into `arr`.
// `other` can be a regular pointer.
// `arr` may be NULL, in which case a new array is created.
DYA_LINKAGE
void* dya_append(void* arr, size_t size, const char other[static size]);

// deallocates and sets `arr` to NULL.
DYA_LINKAGE void dya_free(void* arr);

// Turns the array into a normal pointer that must be freed with normal free().
// The array loses its size and capacity info in the process.
DYA_LINKAGE void* dya_to_pointer(void* arr);

#define dya_reserve(arr, add_size) (arr = (dya_reserve)(arr, add_size))
#define dya_append(arr, size, other)                                           \
    (arr = (dya_append)(arr, size, (const void*) { other }))
#define dya_to_pointer(arr) (arr = (dya_to_pointer)(arr))
#define dya_free(arr) ((dya_free)(arr), arr = 0)

/* ========= MACRO UTILITIES ========= */
// These have `...` to allow for compound initializers.
// Refer to the usage example at the top.

// Works on multidimensional arrays.
// Does nothing if `arr` is NULL.
#define dya_fill(arr, item_type, ...)                                          \
    for (size_t M_VAR(i) = 0, M_VAR(len) = dya_size(arr) / sizeof(item_type);  \
         M_VAR(i) < M_VAR(len); M_VAR(i)++)                                    \
    ((item_type*)arr)[M_VAR(i)] = ((item_type) { __VA_ARGS__ })

// Does not work on multidimensional arrays.
// Creates new array if `arr` is NULL.
#define dya_push(arr, ...)                                                     \
    do {                                                                       \
        dya_reserve(arr, sizeof(*arr));                                        \
        arr[dya_len(arr)] = (__VA_ARGS__);                                     \
        dya_set_size(arr, dya_size(arr) + sizeof(*arr));                       \
    } while (0)

// Only works on multidimensional arrays.
// `row` must be a pointer to data.
#define dya_push_row(arr, row) dya_append(arr, sizeof(*arr), row)

#endif // INCLUDE_DYA_H

/* ========= IMPLEMENTATION ========= */

#define DYA_IMPLEMENTATION
#ifdef DYA_IMPLEMENTATION

#include <string.h> // For memmove() and memset().
#if !defined(DYA_REALLOC) && !defined(DYA_FREE)
#include <stdlib.h>
#define DYA_REALLOC(ptr, size) realloc(ptr, size)
#define DYA_FREE(ptr) free(ptr)
#elif !defined(DYA_REALLOC) || !defined(DYA_FREE)
#error DYA_REALLOC and DYA_FREE must be defined together
#endif

// For internal usage only.
DYA_LINKAGE void* dya_realloc_(void* ptr, size_t size);
DYA_LINKAGE void* dya_base_ptr_(void* arr);
DYA_LINKAGE size_t dya_max_(size_t a, size_t b);

DYA_LINKAGE size_t dya_size(const void* arr)
{
    if (!arr)
        return 0;
    return ((size_t*)arr)[-2];
}

DYA_LINKAGE size_t dya_cap(const void* arr)
{
    if (!arr)
        return 0;
    return ((size_t*)arr)[-1];
}

DYA_LINKAGE void dya_set_size(void* arr, size_t size)
{
    if (!arr)
        return;
    ((size_t*)arr)[-2] = size;
}

DYA_LINKAGE void dya_set_cap(void* arr, size_t cap)
{
    if (!arr)
        return;
    ((size_t*)arr)[-1] = cap;
}

DYA_LINKAGE void*(dya_reserve)(void* arr, size_t add_size)
{
    size_t cap = dya_cap(arr);
    size_t size = dya_size(arr);
    if (size + add_size <= cap)
        return arr;
    size_t new_cap = dya_max_(size + add_size, cap * 2);

    arr = dya_realloc_(arr, new_cap);
    // Need to set these in case arr was NULL
    dya_set_cap(arr, new_cap);
    dya_set_size(arr, size);
    return arr;
}

DYA_LINKAGE void*(dya_append)(void* arr, size_t size,
                              const char other[static size])
{
    if (!other)
        return arr;
    dya_reserve(arr, size);
    memmove((char*)arr + dya_size(arr), other, size);
    dya_set_size(arr, dya_size(arr) + size);
    return arr;
}

DYA_LINKAGE void* dya_alloc(size_t n, size_t row_size)
{
    void* arr = 0;
    dya_reserve(arr, n * row_size);
    dya_set_size(arr, dya_cap(arr));
    return memset(arr, 0, dya_size(arr));
}

DYA_LINKAGE void*(dya_to_pointer)(void* arr)
{
    return memmove(dya_base_ptr_(arr), arr, dya_size(arr));
}

DYA_LINKAGE void(dya_free)(void* arr) { DYA_FREE(dya_base_ptr_(arr)); }

/* ========= IMPLEMENTATION UTILITIES ========= */

#define DYA_OFFSET_ (2 * sizeof(size_t))

DYA_LINKAGE size_t dya_max_(size_t a, size_t b) { return a > b ? a : b; }

DYA_LINKAGE void* dya_base_ptr_(void* arr)
{
    if (!arr)
        return 0;
    return (char*)arr - DYA_OFFSET_;
}

DYA_LINKAGE void* dya_realloc_(void* ptr, size_t size)
{
    void* arr = DYA_REALLOC(dya_base_ptr_(ptr), size + DYA_OFFSET_);
    return (char*)arr + DYA_OFFSET_;
}

#undef DYA_OFFSET_
#endif // DYA_IMPLEMENTATION
