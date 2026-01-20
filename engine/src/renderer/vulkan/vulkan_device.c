#include "vulkan_device.h"

#include <stdlib.h>

#include "core/logger.h"
#include "core/fstring.h"
#include "core/fmemory.h"
#include "containers/darray.h"

// TODO: This could be improved to a bitset. 32 flags should be enough for everything I would probably need in terms of queues.
typedef struct vulkan_physical_device_requirements
{
    bool8_t graphics;
    bool8_t present;
    bool8_t compute;
    bool8_t transfer;
    bool8_t sampler_anisotropy;
    bool8_t discrete_gpu;

    // darray:
    const char** device_extension_names;
} vulkan_physical_device_requirements;

typedef struct vulkan_physical_device_queue_family_info
{
    uint32_t graphics_family_index;
    uint32_t present_family_index;
    uint32_t compute_family_index;
    uint32_t transfer_family_index;
} vulkan_physical_device_queue_family_info;

bool8_t physical_device_meets_requirements(
    VkPhysicalDevice device,
    VkSurfaceKHR surface,
    const VkPhysicalDeviceProperties* properties,
    const VkPhysicalDeviceFeatures* features,
    const vulkan_physical_device_requirements* requirements,
    vulkan_physical_device_queue_family_info* out_queue_info,
    vulkan_swapchain_support_info* out_swapchain_support_info)
{
    // Evaluate device properties to determine if it meets the needs of the application:
    out_queue_info->graphics_family_index = -1;
    out_queue_info->present_family_index = -1;
    out_queue_info->compute_family_index = -1;
    out_queue_info->transfer_family_index = -1;

    // Discrete GPU?
    if (requirements->discrete_gpu)
    {
        if (properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            FINFO("Device is not a discrete GPU, and one is required. Skipping.");
            return FALSE;
        }
    }

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, 0);
    VkQueueFamilyProperties queue_families[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

    // Look at each of the queues and see what queues it supports:
    FINFO("Graphics | Present | Compute | Transfer | Name");
    uint8_t min_transfer_score = 255;
    for (uint32_t i = 0; i < queue_family_count; ++i)
    {
        uint8_t current_transfer_score = 0;

        // Graphics Queue?
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            out_queue_info->graphics_family_index = i;
            ++current_transfer_score;
        }

        // Compute Queue?
        if (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            out_queue_info->compute_family_index = i;
            ++current_transfer_score;
        }

        // Transfer Queue?
        if (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            // Take the index if it is the current lowest.
            // This increases the likelihood that it is a dedicated transfer queue.
            if (current_transfer_score <= min_transfer_score)
            {
                min_transfer_score = current_transfer_score;
                out_queue_info->transfer_family_index = i;
            }
        }

        // Present Queue?
        VkBool32 supports_present = VK_FALSE;
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supports_present));
        if (supports_present)
        {
            out_queue_info->present_family_index = i;
        }
    }

    // Print out some info about the device:
    FINFO("       %d |       %d |       %d |        %d | %s",
        out_queue_info->graphics_family_index != -1,
        out_queue_info->present_family_index != -1,
        out_queue_info->compute_family_index != -1,
        out_queue_info->transfer_family_index != -1,
        properties->deviceName);

    if (
        (!requirements->graphics || (requirements->graphics && out_queue_info->graphics_family_index != -1)) &&
        (!requirements->present || (requirements->present && out_queue_info->present_family_index != -1)) &&
        (!requirements->compute || (requirements->compute && out_queue_info->compute_family_index != -1)) &&
        (!requirements->transfer || (requirements->transfer && out_queue_info->transfer_family_index != -1)))
    {
        FINFO("Device meets queue requirements.");
        FTRACE("Graphics Family Index: %i", out_queue_info->graphics_family_index);
        FTRACE("Present Family Index: %i", out_queue_info->present_family_index);
        FTRACE("Compute Family Index: %i", out_queue_info->compute_family_index);
        FTRACE("Transfer Family Index: %i", out_queue_info->transfer_family_index);

        // Query swapchain support:
        vulkan_device_query_swapchain_support(device, surface, out_swapchain_support_info);

        if (out_swapchain_support_info->format_count < 1 || out_swapchain_support_info->present_mode_count < 1)
        {
            if (out_swapchain_support_info->formats)
            {
                ffree(out_swapchain_support_info->formats,
                    sizeof(VkSurfaceFormatKHR) * out_swapchain_support_info->format_count, MEMORY_TAG_RENDERER);
            }

            if (out_swapchain_support_info->present_modes)
            {
                ffree(out_swapchain_support_info->present_modes,
                    sizeof(VkPresentModeKHR) * out_swapchain_support_info->present_mode_count, MEMORY_TAG_RENDERER);
            }
            FINFO("Required swapchain support not present, skipping device.");
            return FALSE;
        }

        // Device extensions:
        if (requirements->device_extension_names)
        {
            uint32_t available_extension_count = 0;
            VK_CHECK(vkEnumerateDeviceExtensionProperties(device, 0, &available_extension_count, 0));
            if (available_extension_count != 0)
            {
                VkExtensionProperties* available_extensions = 0;
                available_extensions = fallocate(sizeof(VkExtensionProperties) * available_extension_count,
                                                 MEMORY_TAG_RENDERER);
                VK_CHECK(vkEnumerateDeviceExtensionProperties(device, 0, &available_extension_count,
                    available_extensions));

                uint32_t required_extension_count = darray_length(requirements->device_extension_names);
                for (uint32_t i = 0; i < required_extension_count; ++i)
                {
                    bool8_t found = FALSE;
                    for (uint32_t j = 0; j < available_extension_count; ++j)
                    {
                        if (strings_equal(requirements->device_extension_names[i], available_extensions[j].extensionName))
                        {
                            found = TRUE;
                            break;
                        }
                    }

                    if (!found)
                    {
                        FINFO("Required extension not found: '%s', skipping device.", requirements->device_extension_names[i]);
                        ffree(available_extensions, sizeof(VkExtensionProperties) * available_extension_count,
                            MEMORY_TAG_RENDERER);
                        return FALSE;
                    }
                }
                ffree(available_extensions, sizeof(VkExtensionProperties) * available_extension_count,
                    MEMORY_TAG_RENDERER);
            }
        }

        // Sampler Anisotropy:
        if (requirements->sampler_anisotropy && !features->samplerAnisotropy)
        {
            FINFO("Device does not support samplerAnisotropy, skipping.");
            return FALSE;
        }

        // Device meets all requirements:
        return TRUE;
    }

    return FALSE;
}

bool8_t select_physical_device(vulkan_context* context)
{
    uint32_t physical_device_count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physical_device_count, 0));
    if (physical_device_count == 0)
    {
        FFATAL("No devices which support Vulkan were found.");
        return FALSE;
    }

    VkPhysicalDevice physical_devices[physical_device_count];
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physical_device_count, physical_devices));
    for (uint32_t i = 0; i < physical_device_count; ++i)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physical_devices[i], &properties);

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(physical_devices[i], &features);

        VkPhysicalDeviceMemoryProperties memory;
        vkGetPhysicalDeviceMemoryProperties(physical_devices[i], &memory);

        // TODO: These requirements should probably be driven by the engine configuration:
        vulkan_physical_device_requirements requirements = {};
        requirements.graphics = TRUE;
        requirements.present = TRUE;
        requirements.transfer = TRUE;
        // NOTE: Enable this if compute will be required.
        // requirements.compute = TRUE;
        requirements.sampler_anisotropy = TRUE;
        requirements.discrete_gpu = TRUE;
        requirements.device_extension_names = darray_create(const char*);
        darray_push(requirements.device_extension_names, &VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        vulkan_physical_device_queue_family_info queue_info = {};
        bool8_t result = physical_device_meets_requirements(
            physical_devices[i],
            context->surface,
            &properties,
            &features,
            &requirements,
            &queue_info,
            &context->device.swapchain_support_info);

        if (result)
        {
            FINFO("Selected device: '%s'.", properties.deviceName);
            // GPU type, etc:
            switch (properties.deviceType)
            {
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                    FINFO("GPU type is unknown.");
                    break;
                    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    FINFO("GPU type is integrated GPU.");
                    break;
                    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    FINFO("GPU type is discrete GPU.");
                    break;
                    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    FINFO("GPU type is virtual GPU.");
                    break;
                    case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    FINFO("GPU type is CPU.");
                    break;
            }

            FINFO(
                "GPU Driver Version: %d.%d.%d",
                VK_VERSION_MAJOR(properties.driverVersion),
                VK_VERSION_MINOR(properties.driverVersion),
                VK_VERSION_PATCH(properties.driverVersion)
            );

            FINFO(
                "Vulkan API Version: %d.%d.%d",
                VK_VERSION_MAJOR(properties.apiVersion),
                VK_VERSION_MINOR(properties.apiVersion),
                VK_VERSION_PATCH(properties.apiVersion)
            );

            // Memory Information:
            for (uint32_t j = 0; j < memory.memoryHeapCount; ++j)
            {
                float32_t memory_size_gib = (((float32_t)memory.memoryHeaps[j].size) / 1024.0f / 1024.0f / 1014.0f);
                if (memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                {
                    FINFO("Local GPU memory: %.2f GiB", memory_size_gib);
                }
                else
                {
                    FINFO("Shared System memory: %.2f GiB", memory_size_gib);
                }
            }

            context->device.physical_device = physical_devices[i];
            context->device.graphics_queue_family_index = queue_info.graphics_family_index;
            context->device.present_queue_family_index = queue_info.present_family_index;
            context->device.transfer_queue_family_index = queue_info.transfer_family_index;
            // TODO: Set Compute index here if needed:

            // Keep a copy of properties, features and memory info for later use.
            context->device.properties = properties;
            context->device.features = features;
            context->device.memory = memory;
            break;
        }
    }

    // Ensure a device was selected:
    if (!context->device.physical_device)
    {
        FERROR("No physical devices were found which meet the requirements.");
        return FALSE;
    }

    return TRUE;
}

bool8_t vulkan_device_create(vulkan_context* context)
{
    if (!select_physical_device(context))
    {
        return FALSE;
    }

    FINFO("Creating logical device...");

    // N.B: We don't want to create additional queues for shared incides:
    bool8_t present_shares_graphics_queue = context->device.graphics_queue_family_index == context->device.present_queue_family_index;
    bool8_t transfer_shares_graphics_queue = context->device.graphics_queue_family_index == context->device.transfer_queue_family_index;
    uint32_t index_count = 1;

    if (!present_shares_graphics_queue)
    {
        index_count++;
    }

    if (!transfer_shares_graphics_queue)
    {
        index_count++;
    }

    uint32_t indices[index_count];
    uint8_t index = 0;
    indices[index++] = context->device.graphics_queue_family_index;
    if (!present_shares_graphics_queue)
    {
        indices[index++] = context->device.present_queue_family_index;
    }

    if (!transfer_shares_graphics_queue)
    {
        indices[index++] = context->device.transfer_queue_family_index;
    }

    VkDeviceQueueCreateInfo queue_create_infos[index_count];
    for (uint32_t i = 0; i < index_count; ++i)
    {
        queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[i].queueFamilyIndex = indices[i];
        queue_create_infos[i].queueCount = 1;

        // Might not be available for all possible GPUs, however it is available for my current GPU, 1070.
        if (indices[i] == context->device.graphics_queue_family_index)
        {
            queue_create_infos[i].queueCount = 2;
        }

        queue_create_infos[i].flags = 0;
        queue_create_infos[i].pNext = 0;
        float32_t queue_priority = 1.0f;
        queue_create_infos[i].pQueuePriorities = &queue_priority;
    }

    // Request device features:
    // TODO: should be config driven:
    VkPhysicalDeviceFeatures device_features = {};
    device_features.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo device_create_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_create_info.queueCreateInfoCount = index_count;
    device_create_info.pQueueCreateInfos = queue_create_infos;
    device_create_info.pEnabledFeatures = &device_features;
    device_create_info.enabledExtensionCount = 1;
    const char* extension_names = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    device_create_info.ppEnabledExtensionNames = &extension_names;

    // Deprecated and ignored, so passing 0/null:
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = 0;

    // Create the logical device:
    VK_CHECK(vkCreateDevice(context->device.physical_device, &device_create_info, context->allocator,
        &context->device.logical_device));

    FINFO("Logical device successfully created.");

    // Get Queue Handles:
    vkGetDeviceQueue(context->device.logical_device,
        context->device.graphics_queue_family_index, 0, &context->device.graphics_queue);
    vkGetDeviceQueue(context->device.logical_device,
            context->device.present_queue_family_index, 0, &context->device.present_queue);
    vkGetDeviceQueue(context->device.logical_device,
        context->device.transfer_queue_family_index, 0, &context->device.transfer_queue);

    FINFO("Queues Handles successfully obtained from logical device.");

    return TRUE;
}

void vulkan_device_destroy(vulkan_context* context)
{

    // Release/Unset from queue handles:
    context->device.graphics_queue = 0;
    context->device.present_queue = 0;
    context->device.transfer_queue = 0;

    // Destroy logical device:
    FINFO("Destroying logical device...");
    if (context->device.logical_device)
    {
        vkDestroyDevice(context->device.logical_device, context->allocator);
        context->device.logical_device = 0;
    }

    // Physical devices are not destroyed, but released:
    FINFO("Releasing physical device resources...");
    context->device.physical_device = 0;

    // Release formats darray:
    if (context->device.swapchain_support_info.formats)
    {
        ffree(context->device.swapchain_support_info.formats,
            sizeof(VkSurfaceFormatKHR) * context->device.swapchain_support_info.format_count, MEMORY_TAG_RENDERER);
        context->device.swapchain_support_info.formats = 0;
        context->device.swapchain_support_info.format_count = 0;
    }

    // Release present_modes darray:
    if (context->device.swapchain_support_info.present_modes)
    {
        ffree(context->device.swapchain_support_info.present_modes,
            sizeof(VkPresentModeKHR) * context->device.swapchain_support_info.present_mode_count, MEMORY_TAG_RENDERER);
        context->device.swapchain_support_info.present_modes = 0;
        context->device.swapchain_support_info.present_mode_count = 0;
    }

    // Zero out memory for swapchain capabilities:
    fzero_memory(&context->device.swapchain_support_info.capabilities,
        sizeof(context->device.swapchain_support_info.capabilities));

    // Reset queue indices:
    context->device.graphics_queue_family_index = -1;
    context->device.present_queue_family_index = -1;
    context->device.transfer_queue_family_index = -1;
}

void vulkan_device_query_swapchain_support(VkPhysicalDevice physical_device,
    VkSurfaceKHR surface, vulkan_swapchain_support_info* out_support_info)
{
    // Surface capabilities:
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &out_support_info->capabilities));

    // Surface formats:
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &out_support_info->format_count, 0));

    if (out_support_info->format_count != 0)
    {
        if (!out_support_info->formats)
        {
            out_support_info->formats = fallocate(sizeof(VkSurfaceFormatKHR) * out_support_info->format_count,
                MEMORY_TAG_RENDERER);
        }
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &out_support_info->format_count,
            out_support_info->formats));
    }

    // Present modes:
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface,
        &out_support_info->present_mode_count, 0));
    if (out_support_info->present_mode_count != 0)
    {
        if (!out_support_info->present_modes)
        {
            out_support_info->present_modes = fallocate(sizeof(VkPresentModeKHR) * out_support_info->present_mode_count,
                MEMORY_TAG_RENDERER);
        }
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &out_support_info->present_mode_count,
            out_support_info->present_modes));
    }
}

bool8_t vulkan_device_detect_depth_format(vulkan_device* device)
{
    const uint64_t candidate_count = 3;
    VkFormat candidates[3] =
    {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
    };

    uint32_t flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    for (uint64_t i = 0; i < candidate_count; ++i)
    {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(device->physical_device, candidates[i], &properties);

        if ((properties.linearTilingFeatures & flags) == flags)
        {
            device->depth_format = candidates[i];
            return TRUE;
        }
        else if ((properties.optimalTilingFeatures & flags) == flags)
        {
            device->depth_format = candidates[i];
            return TRUE;
        }
    }

    return FALSE;
}