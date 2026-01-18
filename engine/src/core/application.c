#include "application.h"

#include "fmemory.h"
#include "core/clock.h"
#include "core/event.h"
#include "core/input.h"

#include "game_types.h"

#include "platform/platform.h"
#include "logger.h"

#include "renderer/renderer_frontend.h"

typedef struct application_state
{
    clock clock;
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

    // Renderer startup:
    if (!renderer_initialize(game_instance->app_config.name, &app_state.platform))
    {
        FFATAL("Failed to initialize renderer. Aborting application.");
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
    clock_start(&app_state.clock);
    clock_update(&app_state.clock);
    app_state.last_time = app_state.clock.elapsed;

    float64_t running_time = 0;
    uint8_t frame_count = 0;
    float64_t target_frame_seconds = 1.0f / 60.0f;

    FINFO(get_memory_usage_str());

    while (app_state.is_running)
    {
        if (!platform_pump_messages(&app_state.platform))
        {
            app_state.is_running = FALSE;
        }

        if (!app_state.is_suspended)
        {
            // Update clock and get delta time:
            clock_update(&app_state.clock);
            float64_t current_time = app_state.clock.elapsed;
            float64_t delta = (current_time - app_state.last_time);
            float64_t frame_start_time = platform_get_absolute_time();

            if (!app_state.game_instance->update(app_state.game_instance, (float32_t) delta))
            {
                FFATAL("Game update failed, shutting down.");
                app_state.is_running = FALSE;
                break;
            }

            // Call teh game's render routine:
            if (!app_state.game_instance->render(app_state.game_instance, (float32_t) delta))
            {
                FFATAL("Game render failed, shutting down.");
                app_state.is_running = FALSE;
                break;
            }

            // TODO: refactor packet creation
            render_packet packet;
            packet.delta_time = delta;
            renderer_draw_frame(&packet);

            // Figure out how long the frame took:
            float64_t frame_end_time = platform_get_absolute_time();
            float64_t frame_elapsed_time = frame_end_time - frame_start_time;
            running_time += frame_elapsed_time;
            float64_t remaining_seconds = target_frame_seconds - frame_elapsed_time;

            if (remaining_seconds > 0)
            {
                float64_t remaining_ms = (remaining_seconds * 1000);

                // If there is time left, give it back to the OS.
                bool8_t limit_frames = FALSE;
                if (remaining_ms > 0 && limit_frames)
                {
                    platform_sleep(remaining_ms - 1);
                }

                frame_count++;
            }

            // N.B: Input update/state copying should be always handled
            // after any input should be recorded; e.g., before tihs line.
            // As a safety, input is the last thing to be updated before the frame ends.
            input_update(delta);

            // Update last time:
            app_state.last_time = current_time;
        }
    }

    app_state.is_running = FALSE;

    event_unregister(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_unregister(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    event_unregister(EVENT_CODE_KEY_RELEASED, 0, application_on_key);

    event_shutdown();
    input_shutdown();

    renderer_shutdown();

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