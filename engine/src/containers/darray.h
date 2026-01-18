#pragma once

#include "defines.h"

/*
 * Memory layout:
 * uint64_t capacity = number of elements that can be held.
 * uint64_t length = number of elements currently contained.
 * uint64_t stride = size of each element in bytes.
 * void* elements
 */

enum
{
    DARRAY_CAPACITY,
    DARRAY_LENGTH,
    DARRAY_STRIDE,
    DARRAY_FIELD_LENGTH
};

// -- Low-level internal functions --
// These methods take a void** where the ptr might need to be relocated, avoids having to re-assign ptr and having
// to end up with a possible dangling ptr due to a misuse of the container.

FAPI void* _darray_create(uint64_t length, uint64_t stride);
FAPI void _darray_destroy(void* array);

FAPI uint64_t _darray_field_get(void* array, uint64_t field);
FAPI void _darray_field_set(void* array, uint64_t field, uint64_t value);

FAPI void _darray_resize(void** array);

FAPI void _darray_push(void** array, const void* value_ptr);
FAPI void _darray_pop(void** array, void* dest);

FAPI void _darray_pop_at(void** array, uint64_t index, void* dest);
FAPI void _darray_insert_at(void** array, uint64_t index, void* value_ptr);

#define DARRAY_DEFAULT_CAPACITY 1
#define DARRAY_RESIZE_FACTOR 2

#define darray_create(type) \
    _darray_create(DARRAY_DEFAULT_CAPACITY, sizeof(type))

#define darray_reserve(type, capacity) \
    _darray_create(capacity, sizeof(type))

#define darray_destroy(array) _darray_destroy(array)

#define darray_push(array, value)                           \
do {                                                        \
    typeof(value) temp = value;                             \
    _darray_push((void**)&(array), &temp);                  \
} while (0)

#define darray_pop(array, value_ptr)                        \
    _darray_pop((void**)&(array), value_ptr)

#define darray_insert_at(array, index, value)               \
do {                                                        \
    typeof(value) temp = value;                             \
    _darray_insert_at((void**)&(array), index, &temp);      \
} while (0)

#define darray_pop_at(array, index, value_ptr)              \
    _darray_pop_at((void**)&(array), index, value_ptr)

#define darray_clear(array)                                 \
    _darray_field_set(array, DARRAY_LENGTH, 0)

#define darray_capacity(array)                              \
    _darray_field_get(array, DARRAY_CAPACITY)

#define darray_length(array)                                \
    _darray_field_get(array, DARRAY_LENGTH)

#define darray_size(array)                                  \
    darray_length(array);

#define darray_stride(array)                                \
    _darray_field_get(array, DARRAY_STRIDE)

#define darray_length_set(array, value)                     \
    _darray_field_set(array, DARRAY_LENGTH, value)

#define darray_size_set(array, value)                       \
    darray_length_set(array, value)