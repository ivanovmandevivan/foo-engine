#include "platform/platform.h"


// Windows platform layer:
#if FPLATFORM_WINDOWS
#include "core/asserts.h"
#include "core/input.h"
#include "core/logger.h"

#include <windows.h>
#include <windowsx.h> // param input extraction
#include <stdlib.h>

#include "containers/darray.h"

// TODO: Not ideal, exposing vulkan code to the platform win32 layer. Needs to be moved somewhere else.
// For surface creation:
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#include "core/event.h"
#include "renderer/vulkan/vulkan_types.inl"

typedef struct internal_state
{
    HINSTANCE h_instance;
    HWND hwnd;

    // TODO: Not ideal, exposing vulkan code to the platform win32 layer. Needs to be moved somewhere else.
    VkSurfaceKHR surface;
} internal_state;

// Clock (to track application init)
static float64_t clock_frequency;
static LARGE_INTEGER start_time;

LRESULT CALLBACK win32_process_message(HWND hwnd, uint32_t msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg)
    {
        case WM_ERASEBKGND:
            // Notify the OS that erasing will be handled by the application to prevent flicker
            return 1;
        case WM_CLOSE:
            event_context data = {};
            event_fire(EVENT_CODE_APPLICATION_QUIT, 0, data);
            return TRUE;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:
        {
            // // Get updated size:
            // RECT r;
            // GetClientRect(hwnd, &r);
            // uint32_t width = r.right - r.left;
            // uint32_t height = r.bottom - r.top;
            //
            // // TODO: Fire ean event for window resize.
        }
        break;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            // Key Pressed/Released
            bool8_t pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
            keys key = (uint16_t)w_param;

            // Pass to the input subsystem to process:
            input_process_key(key, pressed);
        }
        break;
        case WM_MOUSEMOVE:
        {
            // Mouse move
            int32_t x_position = GET_X_LPARAM(l_param);
            int32_t y_position = GET_Y_LPARAM(l_param);

            input_process_mouse_move(x_position, y_position);
        }
        break;
        case WM_MOUSEWHEEL:
        {
            int32_t zDelta = GET_WHEEL_DELTA_WPARAM(w_param);
            if (zDelta != 0)
            {
                // Flatten the input to an OS-independent (-1, 1);
                zDelta = (zDelta < 0) ? -1 : 1;
                input_process_mouse_wheel(zDelta); // TODO: Narrowing conversion from i32 > i8, fix this
            }
        }
        break;
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        {
            bool8_t pressed = msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN;
            MouseButtons mouse_button = BUTTON_MAX_BUTTONS;
            switch (msg)
            {
                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
                    mouse_button = BUTTON_LEFT;
                    break;
                case WM_MBUTTONDOWN:
                case WM_MBUTTONUP:
                    mouse_button = BUTTON_MIDDLE;
                    break;
                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP:
                    mouse_button = BUTTON_RIGHT;
                    break;
            }

            // Pass over to the input subsystem:
            if (mouse_button != BUTTON_MAX_BUTTONS)
            {
                input_process_mouse_button(mouse_button, pressed);
            }
        }
        break;
    }

    return DefWindowProcA(hwnd, msg, w_param, l_param);
}

bool8_t platform_startup(platform_state* plat_state, const char* application_name, int32_t x, int32_t y,
    int32_t width, int32_t height)
{
    plat_state->internal_state = malloc(sizeof(internal_state));
    internal_state* state = (internal_state*)plat_state->internal_state;

    state->h_instance = GetModuleHandleA(0);

    // Setup and register window class:
    // (https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-wndclassa)
    HICON icon = LoadIcon(state->h_instance, IDI_APPLICATION);
    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_DBLCLKS; // Get double clicks.
    wc.lpfnWndProc = win32_process_message;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = state->h_instance;
    wc.hIcon = icon;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = "foo_window_class";

    if (!RegisterClassA(&wc))
    {
        MessageBoxA(0, "Window registration failed", "Error", MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }

    // Create window:
    uint32_t client_x = x;
    uint32_t client_y = y;
    uint32_t client_width = width;
    uint32_t client_height = height;

    uint32_t window_x = client_x;
    uint32_t window_y = client_y;
    uint32_t window_width = client_width;
    uint32_t window_height = client_height;

    uint32_t window_style = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION;
    uint32_t window_ex_style = WS_EX_APPWINDOW;

    window_style |= WS_MAXIMIZEBOX;
    window_style |= WS_MINIMIZEBOX;
    window_style |= WS_THICKFRAME;

    // Obtain size of border:
    RECT border_rect = {0, 0, 0, 0};
    AdjustWindowRect(&border_rect, window_style, 0);

    // In this case, the border rectangle is negative:
    window_x += border_rect.left;
    window_y += border_rect.top;

    // Grow by the size of the OS Border:
    window_width += border_rect.right - border_rect.left;
    window_height += border_rect.bottom - border_rect.top;

    HWND handle = CreateWindowExA(window_ex_style, "foo_window_class", application_name,
        window_style, window_x, window_y, window_width, window_height, 0, 0,
        state->h_instance, 0);

    if (handle == 0)
    {
        MessageBoxA(NULL, "Window creation failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        FFATAL("Window creation failed!");
        return FALSE;
    }

    state->hwnd = handle;

    // Show the window:
    bool32_t should_activate = 1; // TODO: if the window should not accpet input, this should be false.
    int32_t show_window_command_flags = should_activate ? SW_SHOW : SW_SHOWNOACTIVATE;
    // If initially minimized, use SW_MINIMIZE : SW_SHOWMINNOACTIVE;
    // If initially maximized, use SW_SHOWMAXIMIZED : SW_MAXIMIZE;
    ShowWindow(state->hwnd, show_window_command_flags);

    // Clock start-time setup:
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    clock_frequency = 1.0f / (float64_t)frequency.QuadPart;
    QueryPerformanceCounter(&start_time);

    return TRUE;
}

void platform_shutdown(platform_state* plat_state)
{
    internal_state* state = (internal_state*)plat_state->internal_state;
    if (NULL == state)
    {
        FFATAL("Was not able to retrieve a correct platform state!");
        return;
    }

    FASSERT_MSG(state->hwnd, "No window was found in the specific platform_state, unable to destroy window.");

    DestroyWindow(state->hwnd);
    state->hwnd = 0;
}

bool8_t platform_pump_messages(platform_state* plat_state)
{
    MSG message;
    while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return TRUE;
}

// TODO: For now use stdlib allocation methods, bound for improvement.
void* platform_allocate(uint64_t size, bool8_t aligned)
{
    return malloc(size);
}

void platform_free(void* block, bool8_t* aligned)
{
    free(block);
}

void* platform_zero_memory(void* block, uint64_t size)
{
    return memset(block, 0, size);
}

void* platform_copy_memory(void* dest, const void* source, uint64_t size)
{
    return memcpy(dest, source, size);
}

void* platform_set_memory(void* dest, int32_t value, uint64_t size)
{
    return memset(dest, value, size);
}

void platform_console_write(const char* message, uint8_t color)
{
    HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    // FATAL, ERROR, WARN, INFO, DEBUG, TRACE
    static uint8_t levels[6] = {64, 4, 6, 2, 1, 8};
    SetConsoleTextAttribute(console_handle, levels[color]);

    OutputDebugStringA(message);
    uint64_t length = strlen(message);
    LPDWORD number_written = 0;
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), message, (DWORD)length, number_written, 0);
}

void platform_console_write_error(const char* message, uint8_t color)
{
    HANDLE console_handle = GetStdHandle(STD_ERROR_HANDLE);
    // FATAL, ERROR, WARN, INFO, DEBUG, TRACE
    static uint8_t levels[6] = {64, 4, 6, 2, 1, 8};
    SetConsoleTextAttribute(console_handle, levels[color]);

    OutputDebugStringA(message);
    uint64_t length = strlen(message);
    LPDWORD number_written = 0;
    WriteConsoleA(GetStdHandle(STD_ERROR_HANDLE), message, (DWORD)length, number_written, 0);
}

float64_t platform_get_absolute_time()
{
    LARGE_INTEGER now_time;
    QueryPerformanceCounter(&now_time);
    return (float64_t)now_time.QuadPart * clock_frequency;
}

void platform_sleep(uint64_t ms)
{
    Sleep(ms);
}

void platform_get_required_extension_names(const char*** names_darray)
{
    darray_push(*names_darray, &"VK_KHR_win32_surface");
}

// TODO: Not ideal, exposing vulkan code to the platform win32 layer. Needs to be moved somewhere else.
bool8_t platform_create_vulkan_surface(struct platform_state* plat_state, struct vulkan_context* context)
{
    if (0 == plat_state)
    {
        FFATAL("Platform state is null, unable to create Vulkan Surface!");
        return FALSE;
    }

    if (0 == context)
    {
        FFATAL("Vulkan context is null, unable to create Vulkan Surface!");
        return FALSE;
    }

    internal_state* state = (internal_state*)plat_state->internal_state;

    VkWin32SurfaceCreateInfoKHR create_info = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    create_info.hinstance = state->h_instance;
    create_info.hwnd = state->hwnd;

    VK_CHECK(vkCreateWin32SurfaceKHR(context->instance, &create_info, context->allocator, &state->surface));

    FDEBUG("Vulkan Win32 Surface created successfully.");
    context->surface = state->surface;
    return TRUE;
}

#endif