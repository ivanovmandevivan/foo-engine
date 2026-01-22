#pragma once

#include "vulkan_types.inl"

void vulkan_command_buffer_allocate(vulkan_context* context, VkCommandPool pool, bool8_t is_primary,
    vulkan_command_buffer* out_command_buffer);

void vulkan_command_buffer_free(vulkan_context* context, VkCommandPool pool, vulkan_command_buffer* command_buffer);

void vulkan_command_buffer_begin(vulkan_command_buffer* command_buffer, bool8_t is_single_use,
    bool8_t is_renderpass_continue, bool8_t is_simultaneous_use);

void vulkan_command_buffer_end(vulkan_command_buffer* command_buffer);

void vulkan_command_buffer_update_submitted(vulkan_command_buffer* command_buffer);

void vulkan_command_buffer_reset(vulkan_command_buffer* command_buffer);

/**
 * Allocates and begins recording to out_command_buffer.
 */
void vulkan_command_buffer_allocate_and_begin_single_use(vulkan_context* context, VkCommandPool pool,
    vulkan_command_buffer* out_command_buffer);

/**
 * Ends recording, submits to and waits for queue operation and frees the provided command buffer.
 */
void vulkan_command_buffer_end_single_use(vulkan_context* context, VkCommandPool pool,
    vulkan_command_buffer* command_buffer, VkQueue queue);