#include "darray.h"

#include "core/fmemory.h"
#include "core/logger.h"

void* _darray_create(uint64_t length, uint64_t stride)
{
    uint64_t header_size = DARRAY_FIELD_LENGTH * sizeof(uint64_t);
    uint64_t array_size = length * stride;
    uint64_t* new_array = fallocate(header_size + array_size, MEMORY_TAG_DARRAY);
    fset_memory(new_array, 0, header_size + array_size);
    new_array[DARRAY_CAPACITY] = length;
    new_array[DARRAY_LENGTH] = 0;
    new_array[DARRAY_STRIDE] = stride;
    return (void*)(new_array + DARRAY_FIELD_LENGTH);
}
void _darray_destroy(void* array)
{
    uint64_t* header = (uint64_t*)array - DARRAY_FIELD_LENGTH;
    uint64_t header_size = DARRAY_FIELD_LENGTH * sizeof(uint64_t);
    uint64_t total_size = header_size + (header[DARRAY_CAPACITY] * header[DARRAY_STRIDE]);
    ffree(header, total_size, MEMORY_TAG_DARRAY);
}

uint64_t _darray_field_get(void* array, uint64_t field)
{
    uint64_t* header = (uint64_t*)array - DARRAY_FIELD_LENGTH;
    return header[field];
}
void _darray_field_set(void* array, uint64_t field, uint64_t value)
{
    uint64_t* header = (uint64_t*)array - DARRAY_FIELD_LENGTH;
    header[field] = value;
}

void _darray_resize(void** array)
{
    void* array_val = *array;
    uint64_t length = darray_length(array_val);
    uint64_t stride = darray_stride(array_val);
    void* temp = _darray_create((DARRAY_RESIZE_FACTOR * darray_capacity(array_val)), stride);
    fcopy_memory(temp, array_val, length * stride);

    _darray_field_set(temp, DARRAY_LENGTH, length);
    _darray_destroy(array_val);
    *array = temp;
}

void _darray_push(void** array, const void* value_ptr)
{
    void* array_val = *array;
    uint64_t length = darray_length(array_val);
    uint64_t stride = darray_stride(array_val);
    uint64_t capacity = darray_capacity(array_val);
    if (length >= capacity)
    {
        _darray_resize(array);
        array_val = *array;
    }

    uint64_t addr = (uint64_t)(array_val);
    addr += (length * stride);
    fcopy_memory((void*)addr, value_ptr, stride);
    _darray_field_set(array_val, DARRAY_LENGTH, length + 1);
}
void _darray_pop(void** array, void* dest)
{
    void* array_val = *array;
    uint64_t length = darray_length(array_val);
    if (length <= 0)
    {
        FERROR("Unable to 'pop' this array with length zero! Length: %i", length);
        return;
    }

    uint64_t stride = darray_stride(array_val);
    uint64_t last_element_addr = (uint64_t)(array_val);
    last_element_addr += ((length - 1) * stride);

    if (dest)
    {
        fcopy_memory(dest, (void*)last_element_addr, stride);
    }

    _darray_field_set(array_val, DARRAY_LENGTH, length - 1);
}

void _darray_pop_at(void** array, uint64_t index, void* dest)
{
    void* array_val = *array;
    uint64_t length = darray_length(array_val);
    if (length <= 0)
    {
        FERROR("Unable to 'pop at' this array with length zero! Length: %i, index: %index", length, index);
        return;
    }

    if (index >= length)
    {
        FERROR("Index outside the bounds of this array! Length: %i, index: %index", length, index);
        return;
    }

    uint64_t stride = darray_stride(array_val);
    uint64_t addr = (uint64_t)(array_val);
    fcopy_memory(dest, (void*)(addr + (index * stride)), stride);

    // If not on the last element, snip out the entry and copy the rest inwards:
    if (index != length - 1)
    {
        fcopy_memory(
          (void*)(addr + (index * stride)),
          (void*)(addr + ((index + 1) * stride)),
          stride * (length - index));
    }

    _darray_field_set(array_val, DARRAY_LENGTH, length - 1);
}
void _darray_insert_at(void** array, uint64_t index, void* value_ptr)
{
    void* array_val = *array;
    uint64_t length = darray_length(array_val);
    if (length <= 0)
    {
        FERROR("Unable to 'insert at' this array with length zero! Length: %i, index: %index", length, index);
        return;
    }

    if (index >= length)
    {
        FERROR("Index outside the bounds of this array! Length: %i, index: %index", length, index);
        return;
    }

    if (length >= darray_capacity(array_val))
    {
        _darray_resize(array);
        array_val = *array;
    }

    uint64_t stride = darray_stride(array_val);
    uint64_t addr = (uint64_t)(array_val);

    // If not on the last element, copy the rest outwards:
    if (index != length - 1)
    {
        fcopy_memory(
          (void*)(addr + ((index + 1) * stride)),
          (void*)(addr + (index * stride)),
          stride * (length - index));
    }

    // Set the value at the index:
    fcopy_memory((void*)(addr + (index * stride)), value_ptr, stride);

    _darray_field_set(array_val, DARRAY_LENGTH, length + 1);
}