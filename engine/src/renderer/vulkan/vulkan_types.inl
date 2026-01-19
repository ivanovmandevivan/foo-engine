#pragma once

#include "defines.h"
#include "core/asserts.h"

#include <vulkan/vulkan.h>

// Checks the given expression's return value against VK_SUCCESS.
#define VK_CHECK(expr)                  \
    {                                   \
        FASSERT(expr == VK_SUCCESS);    \
    }

typedef struct vulkan_swapchain_support_info
{
    VkSurfaceCapabilitiesKHR capabilities;
    uint32_t format_count;
    VkSurfaceFormatKHR* formats;
    uint32_t present_mode_count;
    VkPresentModeKHR* present_modes;
} vulkan_swapchain_support_info;

typedef struct vulkan_device
{
    VkPhysicalDevice physical_device;
    VkDevice logical_device;
    vulkan_swapchain_support_info swapchain_support_info;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory;

    uint32_t graphics_queue_family_index;
    uint32_t present_queue_family_index;
    uint32_t transfer_queue_family_index;
} vulkan_device;

typedef struct vulkan_context
{
    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface;

#ifdef _DEBUG
    VkDebugUtilsMessengerEXT debug_messenger;
#endif

    vulkan_device device;
} vulkan_context;