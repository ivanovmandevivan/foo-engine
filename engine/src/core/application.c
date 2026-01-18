#include "application.h"
#include "core/event.h"
#include "core/input.h"

#include "game_types.h"

#include "platform/platform.h"
#include "logger.h"

typedef struct application_state
{
    game* game_instance;
    platform_state platform;
    float64_t last_time;
    int16_t width;
    int16_t height;
    bool8_t is_running;
    bool8_t is_suspended;
} application_state;

static uint8_t initialized = FALSE;
static application_state app_state;

// Event Handlers:
bool8_t application_on_event(uint16_t code, void* sender, void* listener_list, event_context context);
bool8_t application_on_key(uint16_t code, void* sender, void* listener_list, event_context context);

bool8_t application_create(game* game_instance)
{
    if (initialized)
    {
        FERROR("application_create called more than once.");
        return FALSE;
    }

    app_state.game_instance = game_instance;

    // Initialize Subsystems:
    initialize_logging();
    input_initialize();

    // TODO: Remove this
    FFATAL("A test message: %f", 3.14f);
    FERROR("A test message: %f", 3.14f);
    FWARN("A test message: %f", 3.14f);
    FINFO("A test message: %f", 3.14f);
    FDEBUG("A test message: %f", 3.14f);
    FTRACE("A test message: %f", 3.14f);

    app_state.is_running = TRUE;
    app_state.is_suspended = FALSE;

    if (!event_initialize())
    {
        FERROR("Event system failed initialization. Application can't continue");
        return FALSE;
    }

    event_register(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_register(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    event_register(EVENT_CODE_KEY_RELEASED, 0, application_on_key);

    if (!platform_startup(
        &app_state.platform,
        game_instance->app_config.name,
        game_instance->app_config.start_pos_x,
        game_instance->app_config.start_pos_y,
        game_instance->app_config.start_width,
        game_instance->app_config.start_height))
    {
       return FALSE;
    }

    // Initialize the game:
    if (!app_state.game_instance->initialize(app_state.game_instance))
    {
        FFATAL("Game failed to initialize.");
        return FALSE;
    }

    app_state.game_instance->on_resize(app_state.game_instance, app_state.width, app_state.height);

    initialized = TRUE;
    return TRUE;
}

bool8_t application_run()
{
    while (app_state.is_running)
    {
        if (!platform_pump_messages(&app_state.platform))
        {
            app_state.is_running = FALSE;
        }

        if (!app_state.is_suspended)
        {
            if (!app_state.game_instance->update(app_state.game_instance, (float32_t) 0))
            {
                FFATAL("Game update failed, shutting down.");
                app_state.is_running = FALSE;
                break;
            }

            // Call teh game's render routine:
            if (!app_state.game_instance->render(app_state.game_instance, (float32_t) 0))
            {
                FFATAL("Game render failed, shutting down.");
                app_state.is_running = FALSE;
                break;
            }

            // N.B: Input update/state copying should be always handled
            // after any input should be recorded; e.g., before tihs line.
            // As a safety, input is the last thing to be updated before the frame ends.
            input_update(0);
        }
    }

    app_state.is_running = FALSE;

    event_unregister(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_unregister(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    event_unregister(EVENT_CODE_KEY_RELEASED, 0, application_on_key);


    event_shutdown();
    input_shutdown();

    platform_shutdown(&app_state.platform);

    return TRUE;
}

bool8_t application_on_event(uint16_t code, void* sender, void* listener_list, event_context context)
{
    switch (code)
    {
        case EVENT_CODE_APPLICATION_QUIT:
            FINFO("EVENT_CODE_APPLICATION_QUIT received, shutting down.\n");
            app_state.is_running = FALSE;
            return TRUE;
    }

    return FALSE;
}

bool8_t application_on_key(const uint16_t code, void* sender, void* listener_list, event_context context)
{
    if (code == EVENT_CODE_KEY_PRESSED)
    {
        uint16_t key_code = context.data.u16[0];
        if (key_code == KEY_ESCAPE)
        {
            // N.B: Technically firing an event to itself, but there may be other listeners:
            event_context data = {};
            event_fire(EVENT_CODE_APPLICATION_QUIT, 0, data);

            // Block anything else from processing this:
            return TRUE;
        }

        if (key_code == KEY_A)
        {
            // Example on checking for a key:
            FDEBUG("Explicit - A key pressed!");
        }
        else
        {
            FDEBUG("'%c' key pressed in window.", key_code);
        }
    }
    else if (code == EVENT_CODE_KEY_RELEASED)
    {
        uint16_t key_code = context.data.u16[0];
        if (key_code == KEY_B)
        {
            // Example on checking for a key:
            FDEBUG("Explicit - B key released!");
        }
        else
        {
            FDEBUG("'%c' key released in window.", key_code);
        }
    }

    return FALSE;
}