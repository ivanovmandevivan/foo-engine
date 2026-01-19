#pragma once

#include "defines.h"
#include "core/asserts.h"

#include <vulkan/vulkan.h>

// Checks the given expression's return value against VK_SUCCESS.
#define VK_CHECK(expr)                  \
    {                                   \
        FASSERT(expr == VK_SUCCESS);    \
    }

typedef struct vulkan_context
{
    VkInstance instance;
    VkAllocationCallbacks* allocator;

#ifdef _DEBUG
    VkDebugUtilsMessengerEXT debug_messenger;
#endif
} vulkan_context;