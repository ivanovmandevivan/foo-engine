#include "vulkan_backend.h"

#include "vulkan_types.inl"
#include "core/logger.h"
#include "core/fstring.h"
#include "containers/darray.h"
#include "vulkan_platform.h"
#include "vulkan_device.h"

static vulkan_context context;

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data);

bool8_t vulkan_renderer_backend_initialize(renderer_backend* backend, const char* application_name,
    struct platform_state* plat_state)
{
    // TODO: custom allocator
    context.allocator = 0;

    // Setup Vulkan Instance:
    VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app_info.apiVersion = VK_API_VERSION_1_2;
    app_info.pApplicationName = application_name;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Foo Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    VkInstanceCreateInfo create_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    create_info.pApplicationInfo = &app_info;

    // Obtain a list of required extensions:
    const char** required_extensions = darray_create(const char*);
    darray_push(required_extensions, &VK_KHR_SURFACE_EXTENSION_NAME); // Generic surface extension
    platform_get_required_extension_names(&required_extensions); // Platform-specific extension(s)
#ifdef _DEBUG
    darray_push(required_extensions, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // Debug Utils

    FDEBUG("Required extensions:");
    const uint32_t length = darray_length(required_extensions);
    for (int32_t i = 0; i < length; ++i)
    {
        FDEBUG(required_extensions[i]);
    }
#endif

    create_info.enabledExtensionCount = darray_length(required_extensions);
    create_info.ppEnabledExtensionNames = required_extensions;

    // Validation layers:
    const char** required_validation_layer_names = 0;
    uint32_t required_validation_layer_count = 0;

#ifdef _DEBUG
    FINFO("Validation layers enabled. Enumerating:");

    // The list of validation layers required:
    required_validation_layer_names = darray_create(const char*);
    darray_push(required_validation_layer_names, &"VK_LAYER_KHRONOS_validation");
    required_validation_layer_count = darray_length(required_validation_layer_names);

    // Obtain list of available validation layers:
    uint32_t available_layer_count = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, 0));
    VkLayerProperties* available_layers = darray_reserve(VkLayerProperties, available_layer_count);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers));

    // Verify all required layers are available:
    for (uint32_t i = 0; i < required_validation_layer_count; ++i)
    {
        FINFO("Searching for layer: %s...", required_validation_layer_names[i]);
        bool8_t found = FALSE;
        for (uint32_t j = 0; j < available_layer_count; ++j)
        {
            if (strings_equal(required_validation_layer_names[i], available_layers[j].layerName))
            {
                found = TRUE;
                FINFO("Found.");
                break;
            }
        }

        if (!found)
        {
            FFATAL("Required validation layer is missing: %s", required_validation_layer_names[i]);
            return FALSE;
        }
    }
    FINFO("All required validation layers are present.");
#endif

    create_info.enabledLayerCount = required_validation_layer_count;
    create_info.ppEnabledLayerNames = required_validation_layer_names;

    VK_CHECK(vkCreateInstance(&create_info, context.allocator, &context.instance));
    FINFO("Vulkan Instance created.");

#ifdef _DEBUG
    FDEBUG("Creating Vulkan Debugger...");
    uint32_t log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debug_create_info.messageSeverity = log_severity;
    debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debug_create_info.pfnUserCallback = vk_debug_callback;
    debug_create_info.pUserData = 0;

    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkCreateDebugUtilsMessengerEXT");
    FASSERT_MSG(func, "Failed to create Vulkan Debug Messenger!");
    VK_CHECK(func(context.instance, &debug_create_info, context.allocator, &context.debug_messenger));
    FDEBUG("Vulkan Debugger Created Successfully.");
#endif

    // Surface
    FDEBUG("Creating Vulkan Surface...");
    if (!platform_create_vulkan_surface(plat_state, &context))
    {
        FERROR("Failed to create platform surface!");
        return FALSE;
    }
    FDEBUG("Vulkan Surface Successfully Created.");

    // Device Creation:
    if (!vulkan_device_create(&context))
    {
        FERROR("Failed to create device!");
        return FALSE;
    }

    FINFO("Vulkan Renderer initialized successfully.");
    return TRUE;
}

void vulkan_renderer_backend_shutdown(renderer_backend* backend)
{
    // Destroy in the opposite order of creation:
    FINFO("Destroying Vulkan Device...");
    vulkan_device_destroy(&context);

    FINFO("Destroying Vulkan Surface...");
    if (context.surface)
    {
        vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);
        context.surface = 0;
    }

#ifdef _DEBUG
    FDEBUG("Destroying Vulkan Debugger...");
    if (context.debug_messenger)
    {
        PFN_vkDestroyDebugUtilsMessengerEXT func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkDestroyDebugUtilsMessengerEXT");
        func(context.instance, context.debug_messenger, context.allocator);
    }
#endif
    FDEBUG("Destroying Vulkan Instance...");
    vkDestroyInstance(context.instance, context.allocator);
}

void vulkan_renderer_backend_on_resized(renderer_backend* backend, uint16_t width, uint16_t height)
{

}

bool8_t vulkan_renderer_backend_begin_frame(renderer_backend* backend, float32_t delta_time)
{
    return TRUE;
}
bool8_t vulkan_renderer_backend_end_frame(renderer_backend* backend, float32_t delta_time)
{
    return TRUE;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data)
{
    switch (message_severity)
    {
        default:
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            FERROR(callback_data->pMessage);
            break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            FWARN(callback_data->pMessage);
            break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            FINFO(callback_data->pMessage);
            break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            FTRACE(callback_data->pMessage);
            break;
    }
    return VK_FALSE;
}