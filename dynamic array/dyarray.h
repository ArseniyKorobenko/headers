#pragma once
#include "macro_utils.h"
#include <assert.h>
#include <stddef.h> // size_t
// TODO: Start and middle of array insertion and deletion.
/*
 * Notes:
 * Dynamic arrays have 2 additional fields located to the left of their data.
 * Those fields are the capacity of the allocated buffer
 * and the size of the array in bytes.
 * (SIZE_MAX >= capacity >= size)
 *
 * In this header, `size` always refers to size in bytes, and
 * `len` always refers to length in rows.
 * (e.g. for int[5] len would be 5 and size would be 5 * sizeof(int))
 *
 * When an array is extended, additional space may be reserved to prevent
 * frequent reallocations.
 *
 * If an empty array would be created, no allocation is performed and a
 * NULL pointer is returned instead.
 *
 * All functions and macros in this header work with NULLs.
 * If you pass NULL to a function/macro that extends the array,
 * a new array will be allocated and returned.
 *
 * The notion of "rows" used in some macros refers to the data type of array[0].
 *
 * Usage example:
    // Create a 40-wide array of struct tm

    struct tm* timestamps = dya_alloc(40, sizeof *timestamps);
    dya_fill(timestamps, struct tm, .tm_year = 100, .tm_mon = 11 );

    dya_push(timestamps, (struct tm) { .tm_year = 101, .tm_mon = 7 });
    dya_push(timestamps, (struct tm) { .tm_year = 123, .tm_mon = 4 });

    struct tm* timestamps_copy = 0;
    dya_append(timestamps_copy, 5 * sizeof *timestamps, timestamps);
    dya_append(timestamps_copy, dya_size(timestamps), timestamps);

    // Create a 5*10*20 int array of 42s

    int(*arr)[10][20] = dya_alloc(5, sizeof(*arr));
    dya_fill(arr, int, 42);

    arr[2][8][16] += timestamps[10].tm_year;

    int sum = 0;
    dya_foreach(int, iter, arr) {
        sum += *iter;
        *iter = sum;
    }

    dya_free(timestamps_copy);
    dya_free(timestamps);
    dya_free(arr);
 */

size_t dya_size(const void* arr);
#define dya_len(arr) (dya_size(arr) / sizeof *arr)

// Allocate a new array with `n` rows filled with 0.
#define dya_init(n, row_size) dya_alloc(n, row_size)
// Allocate a new array with `n` rows filled with 0.
void* dya_alloc(size_t n, size_t row_size);

/* ========= THESE FUNCTIONS ARE ACTUALLY MACROS ========= */
// These macros reassign `arr` to the return value of the function.

void* dya_set_size(void* arr, size_t new_size);
#define dya_set_len(arr, new_len) dya_set_size(arr, new_len * sizeof *arr)
// `add_size` may be negative.
void* dya_add_size(void* arr, ptrdiff_t add_size);
#define dya_add_len(arr, add_len) dya_add_size(arr, add_len * sizeof *arr)

// Reserve capacity for at least `add_capacity` additional bytes.
void* dya_reserve(void* arr, size_t add_capacity);

// Copies `size` bytes from `other` into `arr`.
// `other` can be a regular pointer of any type.
void* dya_append(void* arr, size_t size, const char other[size]);

// deallocates and sets `arr` to NULL.
void dya_free(void* arr);

// Removes the size and capacity info and turns the array into a vanilla pointer
// that must be freed normally.
// Useful when passing to functions that will try to free() the array.
void* dya_to_pointer(void* arr);

#define dya_set_size(arr, new_size) (arr = (dya_set_size)(arr, new_size))
#define dya_add_size(arr, add_size) (arr = (dya_add_size)(arr, add_size))
#define dya_reserve(arr, add_cap) (arr = (dya_reserve)(arr, add_cap))
#define dya_to_pointer(arr) (arr = (dya_to_pointer)(arr))
#define dya_append(arr, size, other)                                           \
    (arr = (dya_append)(arr, size, (const void*) { other }))
#define dya_free(arr) ((dya_free)(arr), arr = 0)

/* ========= MACRO UTILITIES ========= */
// These have `...` to allow for compound initializers.

// clang-format off

// Iterate over `arr`s elements. `iter` is an `item_type` pointer.
#define dya_foreach(item_type, iter, arr)                                      \
    for (item_type *iter = 0,                                                  \
                   *M_VAR(i) = (item_type*)(arr),                              \
                   *M_VAR(end) = M_VAR(i)                                      \
                               + dya_size(M_VAR(i)) / sizeof(item_type);       \
         (iter = M_VAR(i)) < M_VAR(end); M_VAR(i)++)

// Iterate in reverse over `arr`s elements. `iter` is an `item_type` pointer.
#define dya_foreachr(item_type, iter, arr)                                     \
    for (item_type *iter = 0,                                                  \
                   *M_VAR(start) = (item_type*)(arr),                          \
                   *M_VAR(i) = M_VAR(start)                                    \
                             + dya_size(M_VAR(start)) / sizeof(item_type) - 1; \
         M_VAR(start) && (iter = M_VAR(i)) >= M_VAR(start); M_VAR(i)--)

// Works on multidimensional arrays.
#define dya_fill(arr, item_type, /*item*/...)                                  \
    dya_foreach (item_type, M_VAR(iter), arr)                                  \
    *M_VAR(iter) = ((item_type) { __VA_ARGS__ })

// Does not work on multidimensional arrays. Use dya_append() for that.
#define dya_push(arr, /*item*/...)                                             \
    do {                                                                       \
        dya_add_len(arr, 1);                                                   \
        arr[dya_len(arr) - 1] = (__VA_ARGS__);                                 \
    } while (0)

#define dya_pop(arr) (dya_add_len(arr, -1), arr[dya_len(arr)])

// clang-format on
