#include "core/fstring.h"
#include "core/fmemory.h"

#include <string.h>

uint64_t string_length(const char* str)
{
    return strlen(str);
}

char* string_duplicate(const char* str)
{
    uint64_t length = string_length(str);
    char* copy = fallocate(length + 1, MEMORY_TAG_STRING);
    fcopy_memory(copy, str, length + 1);
    return copy;
}

bool8_t strings_equal(const char* str0, const char* str1)
{
    return strcmp(str0, str1) == 0;
}
