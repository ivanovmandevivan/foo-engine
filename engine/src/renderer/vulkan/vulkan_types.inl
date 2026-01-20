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

    VkQueue graphics_queue;
    VkQueue present_queue;
    VkQueue transfer_queue;

    VkFormat depth_format;
} vulkan_device;

typedef struct vulkan_image
{
    VkImage handle;
    VkDeviceMemory memory;
    VkImageView view;
    uint32_t width;
    uint32_t height;
} vulkan_image;

typedef struct vulkan_swapchain
{
    vulkan_image depth_attachment;
    VkImage* images;
    VkImageView* views;
    VkSurfaceFormatKHR image_format;
    VkSwapchainKHR handle;
    uint32_t image_count;
    uint8_t max_frames_in_flight;
} vulkan_swapchain;

typedef struct vulkan_context
{
    vulkan_device device;
    vulkan_swapchain swapchain;
    int32_t (*find_memory_index)(uint32_t type_filter, uint32_t property_flags);

    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface;

#ifdef _DEBUG
    VkDebugUtilsMessengerEXT debug_messenger;
#endif

    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint32_t image_index;
    uint32_t current_frame;
    bool8_t recreating_swapchain;
} vulkan_context;