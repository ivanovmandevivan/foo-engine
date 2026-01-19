#pragma once

#include "defines.h"

struct platform_state;
struct vulkan_context;

/***
 * Appends the names of the required extensions for this platform to the names_darray, which should be created
 * and passed in.
 */
void platform_get_required_extension_names(const char*** names_darray);

bool8_t platform_create_vulkan_surface(struct platform_state* plat_state, struct vulkan_context* context);