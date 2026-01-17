#include <pthread_time.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "defines.h"

// Linux platform layer:
#if FPLATFORM_LINUX

#include "core/logger.h"

#include <xcb/xcb.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h> // sudo apt-get install libx11-dev
#include <X11/Xlib.h>
#include <X11/xlib-xcb.h> // sudo apt-get install libxkbcommon-x11-dev
#include <sys/time.h>

#if _POSIX_C_SOURCE >= 199309L
#include <time.h> // nanosleep
#else
#include <unistd.h> // usleep
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct internal_state
{
    Display* display;
    xcb_connection_t* connection;
    xcb_window_t window;
    xcb_screen_t* screen;
    xcb_atom_t wm_protocols;
    xcb_atom_t wm_delete_win;
} internal_state;

bool8_t platform_startup(platform_state* plat_state, const char* application_name, int32_t x, int32_t y,
    int32_t width, int32_t height)
{
    // Create the internal state.
    plat_state->internal_state = malloc(sizeof(internal_state));
    internal_state* state = (internal_state*)plat_state->internal_state;

    // Connect to X:
    state->display = XOpenDisplay(NULL);

    // Turn off key repeats:
    // TODO: This might be global, can't make it local to the application if you forget to turn it on. Need to find a fix.
    XAutoRepeatOff(state->display);

    // Retrieve connection from display:
    state->connection = XGetXCBConnection(state->display);

    if (xcb_connection_has_error(state->connection))
    {
        FFATAL("Failed to connect to X server via XCB.");
        return FALSE;
    }

    // Get data from the X server:
    const struct xcb_setup_t* setup = xcb_get_setup(state->connection);

    // Loop through screens using iterator
    xcb_screen_iterator_t it = xcb_setup_roots_iterator(setup);
    int32_t screen_p = 0;
    for  (int32_t s = screen_p; s > 0; s--)
    {
        xcb_screen_next(&it);
    }

    // After screens have been looped through, assign it:
    state->screen = it.data;

    // Allocate a XID for the window to be created:
    state->window = xcb_generate_id(state->connection);

    // Register event types.
    // XCB_CW_BACK_PIXEL = filling then window bg with a single color.
    // XCB_CW_EVENT_MASK is required.
    uint32_t event_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;

    // Listen for keyboard and mouse buttons:
    uint32_t event_values = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_KEY_PRESS |
        XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE |
        XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    // Values to be sent over XCB (bg color, events):
    uint32_t value_list[] = (state->screen->black_pixel, event_values);

    // Create the window:
    xcb_void_cookie_t cookie = xcb_create_window(state->connection, XCB_COPY_FROM_PARENT, state->window,
        state->screen->root, x, y, width, height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, state->screen->root_visual,
        event_mask, value_list);

    // Change the title of the window:
    xcb_change_property(state->connection, XCB_PROP_MODE_REPLACE, state->window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
        strlen(application_name), application_name);

    // Tell the server to notify when the window manager attempts to destroy the window:
    xcb_intern_atom_cookie_t wm_delete_cookie = xcb_intern_atom(state->connection, 0, strlen("WM_DELETE_WINDOW"),
        "WM_DELETE_WINDOW");
    xcb_intern_atom_cookie_t wm_protocols_cookie = xcb_intern_atom(state->connection, 0, strlen("WM_PROTOCOLS"),
        "WM_PROTOCOLS");
    xcb_intern_atom_reply_t* wm_delete_reply = xcb_intern_atom_reply(state->connection, wm_delete_cookie, NULL);
    xcb_intern_atom_reply_t* wm_protocols_reply = xcb_intern_atom_reply(state->connection, wm_protocols_cookie, NULL);
    state->wm_delete_win = wm_delete_reply->atom;
    state->wm_protocols = wm_protocols_reply->atom;

    xcb_change_property(state->connection, XCB_PROP_MOVE_REPLACE, state->window, wm_protocols_reply->atom, 4, 32, 1,
        &wm_delete_reply->atom);

    // Map the window to the screen:
    xcb_map_window(state->connection, state->window);

    // Flush the stream:
    int32_t stream_result = xcb_flush(state->connection);
    if (stream_result <= 0)
    {
        FFATAL("An error ocurred when flushing the stream: %d", stream_result);
        return FALSE;
    }

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

    FASSERT_MSG(state->window, "No window was found in the specific platform_state, unable to destroy window.");

    // Turn key repeats back on since this is global for the OS.
    XAutoRepeatOn(state->display);

    xcb_destroy_window(state->connection, state->window);
}

bool8_t platform_pump_messages(platform_state* plat_state)
{
    internal_state* state = (internal_state*)plat_state->internal_state;
    if (NULL == state)
    {
        FFATAL("Was not able to retrieve a correct platform state!");
        return;
    }

    xcb_generic_event_t* event;
    xcb_client_message_event_t* cm;

    bool8_t quit_flagged = FALSE;

    // Poll for events until null is returned:
    while (event != 0)
    {
        event = xcb_poll_for_event(state->connection);
        if (event == 0)
        {
            break;
        }

        // Input events:
        switch (event->response_type & ~0x80)
        {
            case XCB_KEY_PRESS:
            case XCB_KEY_RELEASE:
            {
                // TODO: Key presses and releases
            }
            break;
            case XCB_BUTTON_PRESS:
            case XCB_BUTTON_RELEASE:
            {
                // TODO: Mouse button presses and releases
            }
            break;
            case XCB_MOTION_NOTIFY:
                // TODO: mouse movement.
            break;
            case XCB_CONFIGURE_NOTIFY:
            {
                // TODO: Resizing
            }
            break;
            case XCB_CLIENT_MESSAGE:
            {
                cm = (xcb_client_message_event_t*)event;

                // Window close:
                if (cm->data.data32[0] == state->wm_delete_win)
                {
                    quit_flagged = TRUE;
                }
            }
            break;
            default:
                // Something else
                break;
        }

        free(event);
    }

    return !quit_flagged;
}

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
    // FATAL, ERROR, WARN, INFO, DEBUG, TRACE
    const char* color_strings[] = {"0;41", "1;31", "1;33", "1;32", "1;34", "1;30"};
    printf("\033[%sm%s\033[0m", color_strings[color], message);
}

void platform_console_write_error(const char* message, uint8_t color)
{
    // FATAL, ERROR, WARN, INFO, DEBUG, TRACE
    const char* color_strings[] = {"0;41", "1;31", "1;33", "1;32", "1;34", "1;30"};
    printf("\033[%sm%s\033[0m", color_strings[color], message);
}

float64_t platform_get_absolute_time()
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec + now.tv_nsec * 0.000000001;
}

// Sleep on the thread for the provided ms. This blocks the main thread.auto
// Should only be used for giving time back to the OS for unused update power.
// Therefore, it is not exported.
void platform_sleep(uint64_t ms)
{
#if _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000 * 1000;
    nanosleep(&ts, 0);
#else
    if (ms >= 1000)
    {
        sleep(ms / 1000);
    }
    usleep((ms % 1000) * 1000);
#endif
}

#endif