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

typedef enum vulkan_render_pass_state
{
    READY,
    RECORDING,
    IN_RENDER_PASS,
    RECORDING_ENDED,
    SUBMITTED,
    NOT_ALLOCATED
} vulkan_render_pass_state;

typedef struct vulkan_renderpass
{
    VkRenderPass handle;
    float32_t x, y, w, h;
    float32_t r, g, b, a;

    float32_t depth;
    uint32_t stencil;

    vulkan_render_pass_state state;
} vulkan_renderpass;

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

typedef enum vulkan_command_buffer_state
{
    COMMAND_BUFFER_STATE_READY,
    COMMAND_BUFFER_STATE_RECORDING,
    COMMAND_BUFFER_STATE_IN_RENDER_PASS,
    COMMAND_BUFFER_STATE_RECORDING_ENDED,
    COMMAND_BUFFER_STATE_SUBMITTED,
    COMMAND_BUFFER_STATE_NOT_ALLOCATED
} vulkan_command_buffer_state;

typedef struct vulkan_command_buffer
{
    VkCommandBuffer handle;

    // Command buffer state:
    vulkan_command_buffer_state state;
} vulkan_command_buffer;

typedef struct vulkan_context
{
    vulkan_device device;
    vulkan_swapchain swapchain;
    vulkan_renderpass main_renderpass;
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