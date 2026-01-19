#pragma once

#include "defines.h"

// Return the length of the given string:
FAPI uint64_t string_length(const char* str);
FAPI char* string_duplicate(const char* str);

// Case-sensitive comparison. True if same, false if not.
FAPI bool8_t strings_equal(const char* str0, const char* str1);