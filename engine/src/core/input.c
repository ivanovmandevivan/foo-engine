#include "core/input.h"
#include "core/event.h"
#include "core/fmemory.h"
#include "core/logger.h"

typedef struct keyboard_state
{
    bool8_t keys[256];
} keyboard_state;

typedef struct mouse_State
{
    int32_t x;
    int32_t y;
    uint8_t buttons[BUTTON_MAX_BUTTONS];
} mouse_state;

typedef struct input_state
{
    keyboard_state keyboard_current;
    keyboard_state keyboard_previous;
    mouse_state mouse_current;
    mouse_state mouse_previous;
} input_state;

// Internal input state:
static bool8_t initialized = FALSE;
static input_state state = {};

void input_initialize()
{
    fzero_memory(&state, sizeof(input_state));
    initialized = TRUE;
    FINFO("Input Subsystem Initialized.");
}

void input_shutdown()
{
    // TODO: Add shutdown routines when needed.
    initialized = FALSE;
}

void input_update(float64_t delta_time)
{
    if (!initialized)
    {
        return;
    }

    // Copy current states to previous states:
    fcopy_memory(&state.keyboard_previous, &state.keyboard_current, sizeof(keyboard_state));
    fcopy_memory(&state.mouse_previous, &state.mouse_current, sizeof(mouse_state));
}

void input_process_key(keys keyCode, bool8_t pressed)
{
    // Only handle if the state actually has changed:
    if (state.keyboard_current.keys[keyCode] != pressed)
    {
        // Update internal state:
        state.keyboard_current.keys[keyCode] = pressed;

        // Fire off an event for immediate processing:
        event_context context;
        context.data.u16[0] = keyCode;
        event_fire(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, 0, context);
    }
}

void input_process_mouse_button(MouseButtons button, bool8_t pressed)
{
   // If the state has changed, fire an event:
    if (state.mouse_previous.buttons[button] != pressed)
    {
        state.mouse_previous.buttons[button] = pressed;

        // Fire the event:
        event_context context;
        context.data.u16[0] = button;
        event_fire(pressed ? EVENT_CODE_MOUSE_BUTTON_PRESSED : EVENT_CODE_MOUSE_BUTTON_RELEASED, 0, context);
    }
}

void input_process_mouse_move(int32_t x, int32_t y)
{
    // Only process if actually different:
    if (state.mouse_current.x != x || state.mouse_current.y != y)
    {
        // N.B: Enable this if debugging:
        // FDEBUG("Mouse Pos: %i, %i!", x, y);

        // Update internal state:
        state.mouse_current.x = x;
        state.mouse_current.y = y;

        // Fire the event:
        event_context context;
        context.data.u32[0] = x;
        context.data.u32[1] = y;
        event_fire(EVENT_CODE_MOUSE_MOVED, 0, context);
    }
}

void input_process_mouse_wheel(int8_t z_delta)
{
    // N.B: No internal state to update.

    // Fire the event:
    event_context context;
    context.data.u8[0] = z_delta;
    event_fire(EVENT_CODE_MOUSE_WHEEL, 0, context);
}

bool8_t input_is_key_down(keys keyCode)
{
    if (!initialized)
    {
        return FALSE;
    }
    return state.keyboard_current.keys[keyCode] == TRUE;
}

bool8_t input_is_key_up(keys keyCode)
{
    if (!initialized)
    {
        return TRUE;
    }
    return state.keyboard_current.keys[keyCode] == FALSE;
}

bool8_t input_was_key_down(keys keyCode)
{
    if (!initialized)
    {
        return FALSE;
    }
    return state.keyboard_previous.keys[keyCode] == TRUE;
}

bool8_t input_was_key_up(keys keyCode)
{
    if (!initialized)
    {
        return TRUE;
    }
    return state.keyboard_previous.keys[keyCode] == FALSE;
}

bool8_t input_is_mouse_button_down(MouseButtons button)
{
    if (!initialized)
    {
        return FALSE;
    }
    return state.mouse_current.buttons[button] == TRUE;
}

bool8_t input_is_mouse_button_up(MouseButtons button)
{
    if (!initialized)
    {
        return TRUE;
    }
    return state.mouse_current.buttons[button] == FALSE;
}

bool8_t input_was_mouse_button_down(MouseButtons button)
{
    if (!initialized)
    {
        return FALSE;
    }
    return state.mouse_previous.buttons[button] == TRUE;
}

bool8_t input_was_mouse_button_up(MouseButtons button)
{
    if (!initialized)
    {
        return TRUE;
    }
    return state.mouse_previous.buttons[button] == FALSE;
}

void input_get_mouse_position(int32_t* x, int32_t* y)
{
    if (!initialized)
    {
        *x = 0;
        *y = 0;
        return;
    }

    *x = state.mouse_current.x;
    *y = state.mouse_current.y;
}

void input_get_previous_mouse_position(int32_t* x, int32_t* y)
{
    if (!initialized)
    {
        *x = 0;
        *y = 0;
        return;
    }

    *x = state.mouse_previous.x;
    *y = state.mouse_previous.y;
}
