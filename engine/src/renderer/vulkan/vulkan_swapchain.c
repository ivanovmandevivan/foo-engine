#include "vulkan_swapchain.h"

#include "core/logger.h"
#include "core/fmemory.h"
#include "vulkan_device.h"
#include "vulkan_image.h"

// -- Internal Methods --

void create(vulkan_context* context, uint32_t width, uint32_t height, vulkan_swapchain* swapchain);
void destroy(vulkan_context* context, vulkan_swapchain* swapchain);


// -- API Methods --
void vulkan_swapchain_create(vulkan_context* context, uint32_t width, uint32_t height, vulkan_swapchain* swapchain)
{
    create(context, width, height, swapchain);
}

void vulkan_swapchain_recreate(vulkan_context* context, uint32_t width, uint32_t height, vulkan_swapchain* swapchain)
{
    destroy(context, swapchain);
    create(context, width, height, swapchain);
}

void vulkan_swapchain_destroy(vulkan_context* context, vulkan_swapchain* swapchain)
{
    destroy(context, swapchain);
}

bool8_t vulkan_swapchain_acquire_next_image_index(vulkan_context* context, vulkan_swapchain* swapchain,
    uint64_t timeout_ns, VkSemaphore image_available_semaphore, VkFence fence, uint32_t* out_image_index)
{
    VkResult result = vkAcquireNextImageKHR(context->device.logical_device, swapchain->handle, timeout_ns,
        image_available_semaphore, fence, out_image_index);

    // TODO: Revisit this in the future to try to specifically palliate each individual fail result.
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        // Trigger swapchain recreation, then boot out of the render loop.
        vulkan_swapchain_recreate(context, context->framebuffer_width, context->framebuffer_height, swapchain);
        return FALSE;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        FFATAL("Failed to acquire swapchain image!");
        return FALSE;
    }

    return TRUE;
}

void vulkan_swapchain_present(vulkan_context* context, vulkan_swapchain* swapchain, VkQueue graphics_queue,
    VkQueue present_queue, VkSemaphore render_complete_semaphore, uint32_t present_image_index)
{
    // Return the image to the swapchain for presentation at the index provided.
    VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &render_complete_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain->handle;
    present_info.pImageIndices = &present_image_index;
    present_info.pResults = 0;

    // TODO: Revisit this in the future to try to specifically palliate each individual fail result.
    VkResult result = vkQueuePresentKHR(present_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        // Swapchain is out of date, suboptimal or a framebuffer resize has occured. Trigger swapchain recreation:
        vulkan_swapchain_recreate(context, context->framebuffer_width, context->framebuffer_height, swapchain);
    }
    else if (result != VK_SUCCESS)
    {
        FFATAL("Failed to present swapchain image!");
    }
}

// --- Internal Methods Impl ---
void create(vulkan_context* context, uint32_t width, uint32_t height, vulkan_swapchain* swapchain)
{
    VkExtent2D swapchain_extent = {width, height};

    // Use triple buffering if we can. (2 frames being rendered two, as we draw a third frame)
    // fallback to double buffering it triple buffering failed.
    swapchain->max_frames_in_flight = 2;

    // Choose a swap surface format:
    bool8_t found = FALSE;
    for (uint32_t i = 0; i < context->device.swapchain_support_info.format_count; ++i)
    {
        VkSurfaceFormatKHR format = context->device.swapchain_support_info.formats[i];
        // Preferred formats:
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            swapchain->image_format = format;
            found = TRUE;
            break;
        }
    }

    if (!found)
    {
        swapchain->image_format = context->device.swapchain_support_info.formats[0];
    }

    // By default, use FIFO_KHR because it is guaranteed to exist.
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t i = 0; i < context->device.swapchain_support_info.present_mode_count; ++i)
    {
        VkPresentModeKHR mode = context->device.swapchain_support_info.present_modes[i];
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            present_mode = mode;
            break;
        }
    }

    // Requery swapchain support.
    vulkan_device_query_swapchain_support(context->device.physical_device, context->surface,
        &context->device.swapchain_support_info);

    // Swapchain extent:
    if (context->device.swapchain_support_info.capabilities.currentExtent.width != UINT32_MAX)
    {
        swapchain_extent = context->device.swapchain_support_info.capabilities.currentExtent;
    }

    // Clamp to the value allowed by the GPU:
    VkExtent2D min = context->device.swapchain_support_info.capabilities.minImageExtent;
    VkExtent2D max = context->device.swapchain_support_info.capabilities.maxImageExtent;
    swapchain_extent.width = FCLAMP(swapchain_extent.width, min.width, max.width);
    swapchain_extent.height = FCLAMP(swapchain_extent.height, min.height, max.height);

    uint32_t image_count = context->device.swapchain_support_info.capabilities.minImageCount + 1;
    if (context->device.swapchain_support_info.capabilities.maxImageCount > 0 &&
        image_count > context->device.swapchain_support_info.capabilities.maxImageCount)
    {
        image_count = context->device.swapchain_support_info.capabilities.maxImageCount;
    }

    // Swapchain create info:
    VkSwapchainCreateInfoKHR swapchain_create_info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchain_create_info.surface = context->surface;
    swapchain_create_info.minImageCount = image_count;
    swapchain_create_info.imageFormat = swapchain->image_format.format;
    swapchain_create_info.imageColorSpace = swapchain->image_format.colorSpace;
    swapchain_create_info.imageExtent = swapchain_extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // Setup the queue family indices:
    if (context->device.graphics_queue_family_index != context->device.present_queue_family_index)
    {
        uint32_t queueFamilyIndices[] = {
            context->device.graphics_queue_family_index,
            context->device.present_queue_family_index,
        };
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
        swapchain_create_info.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.queueFamilyIndexCount = 0;
        swapchain_create_info.pQueueFamilyIndices = 0;
    }

    // Portrait vs Landscape for the presentation:
    swapchain_create_info.preTransform = context->device.swapchain_support_info.capabilities.currentTransform;

    // Compositing mode, we want it to be opaque, no transparency in compositing (with the OS):
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = present_mode;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = 0;

    VK_CHECK(vkCreateSwapchainKHR(context->device.logical_device, &swapchain_create_info, context->allocator,
        &swapchain->handle));

    FINFO("Swapchain has been created successfully.");

    // Start with a zero frame index:
    context->current_frame = 0;

    // Images:
    swapchain->image_count = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(context->device.logical_device, swapchain->handle, &swapchain->image_count, 0));
    if (!swapchain->images)
    {
        swapchain->images = (VkImage*)fallocate(sizeof(VkImage) * swapchain->image_count, MEMORY_TAG_RENDERER);
    }

    if (!swapchain->views)
    {
        swapchain->views = (VkImageView*)fallocate(sizeof(VkImageView) * swapchain->image_count, MEMORY_TAG_RENDERER);
    }

    VK_CHECK(vkGetSwapchainImagesKHR(context->device.logical_device, swapchain->handle, &swapchain->image_count,
        swapchain->images));

    // Create image views (one for each image provided by the swapchain):
    for (uint32_t i = 0; i < swapchain->image_count; i++)
    {
        VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view_info.image = swapchain->images[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = swapchain->image_format.format;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(context->device.logical_device, &view_info, context->allocator, &swapchain->views[i]));
    }

    // Depth resources:
    if (!vulkan_device_detect_depth_format(&context->device))
    {
        context->device.depth_format = VK_FORMAT_UNDEFINED;
        FFATAL("Failed to find a supported format!");
    }

    // Create an image (since swapchain doesn't provide a depth image for us):
    vulkan_image_create(
      context,
      VK_IMAGE_TYPE_2D,
      swapchain_extent.width,
      swapchain_extent.height,
      context->device.depth_format,
      VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      TRUE,
      VK_IMAGE_ASPECT_DEPTH_BIT,
      &swapchain->depth_attachment);

    FINFO("Depth Attachment created successfully.");
}

void destroy(vulkan_context* context, vulkan_swapchain* swapchain)
{
    vulkan_image_destroy(context, &swapchain->depth_attachment);

    // Only destroy the views, not the images, sinze those are owned by the swapchain and are thus destroyed when it is.
    for (uint32_t i = 0; i < swapchain->image_count; ++i)
    {
        vkDestroyImageView(context->device.logical_device, swapchain->views[i], context->allocator);
    }

    vkDestroySwapchainKHR(context->device.logical_device, swapchain->handle, context->allocator);
}