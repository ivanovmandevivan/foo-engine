#pragma once

#include "defines.h"

typedef struct event_context
{
    // 128 bytes
    union
    {
        int64_t i64[2];
        uint64_t u64[2];
        float64_t f64[2];

        int32_t i32[4];
        uint32_t u32[4];
        float32_t f32[4];

        int16_t i16[8];
        uint16_t u16[8];

        int8_t i8[16];
        uint8_t u8[16];

        char c[16];
    } data;
} event_context;

// Should return true if handled:
typedef bool8_t (*ptrfn_on_event)(uint16_t code, void* sender, void* listener_list, event_context data);

bool8_t event_initialize();
void event_shutdown();

/**
 * Register to listen for when events are sent with the provided code. Events with duplicate listener/callback combos
 * will not be registered again and will cause this to return FALSE.
 * @param code The event code to listen for.
 * @param listener A pointer to a listener instance. Can be 0/NULL.
 * @param on_event The callback function ptr to be invoked when the event code is fired.
 * @returns TRUE if the event is successfully registered; otherwise FALSE.
 */
FAPI bool8_t event_register(uint16_t code, void* listener, ptrfn_on_event on_event);

/**
 * Unregister from listening for when events are sent with the provided code. If no matching registration is found, this
 * function returns FALSE.
 * @param code The event code to stop listening for.
 * @param listener A pointer to a listener instance. Can be 0/NULL.
 * @param on_event The callback function ptr to be unregistered.
 * @returns TRUE if the event is successfully unregistered; otherwise FALSE.
 */
FAPI bool8_t event_unregister(uint16_t code, void* listener, ptrfn_on_event on_event);

/**
 * Fires an event to listeners of the given code. If an event handler returns TRUE, the event is considered handled and
 * is not passed on to any more listeners.
 * @param code The event code to fire.
 * @param sender A pointer to the sender. Can be 0/NULL.
 * @param data The event data.
 * @returns TRUE if handled, otherwise FALSE.
 */
FAPI bool8_t event_fire(uint16_t code, void* sender, event_context context);

// System internal event codes. Application should use codes beyond 255.
typedef enum system_event_code
{
    // Shuts down the application down on the next frame:
    EVENT_CODE_APPLICATION_QUIT = 0x01,

    // Keyboard key pressed.
    /*
     * Context usage:
     * uint16_t key_code = data.data.u16[0];
     */
    EVENT_CODE_KEY_PRESSED = 0x02,

    // Keyboard key released.
    /*
     * Context usage:
     * uint16_t key_code = data.data.u16[0];
     */
    EVENT_CODE_KEY_RELEASED = 0x03,

    // Mouse button pressed.
    /*
     * Context usage:
     * uint16_t mbutton = data.data.u16[0];
     */
    EVENT_CODE_MOUSE_BUTTON_PRESSED = 0x04,

    // Mouse button released.
    /*
     * Context usage:
     * uint16_t mbutton = data.data.u16[0];
     */
    EVENT_CODE_MOUSE_BUTTON_RELEASED = 0x05,

    // Mouse moved.
    /*
     * Context usage:
     * uint16_t x = data.data.u16[0];
     * uint16_t y = data.data.u16[1];
     */
    EVENT_CODE_MOUSE_MOVED = 0x06,

    // Mouse wheel moved.
    /*
     * Context usage:
     * uint8_t z_delta = data.data.u8[0];
     */
    EVENT_CODE_MOUSE_WHEEL = 0x07,

    // Resize/resolution changed from the OS.
    /*
     * Context usage:
     * uint16_t width = data.data.u16[0];
     * uint16_t height = data.data.u16[1];
     */
    EVENT_CODE_RESIZE = 0x08,

    MAX_EVENT_CODE = 0xFF
} system_event_code;