#pragma once

#include "vulkan_types.inl"

void vulkan_renderpass_create(vulkan_context* context, vulkan_renderpass* out_renderpass, float32_t x, float32_t y,
    float32_t w, float32_t h, float32_t r, float32_t g, float32_t b, float32_t a, float32_t depth, uint32_t stencil);

void vulkan_renderpass_destroy(vulkan_context* context, vulkan_renderpass* renderpass);

void vulkan_renderpass_begin(vulkan_command_buffer* command_buffer, vulkan_renderpass* renderpass,
    VkFramebuffer frame_buffer);

void vulkan_renderpass_end(vulkan_command_buffer* command_buffer, vulkan_renderpass* renderpass);