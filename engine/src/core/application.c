#include "application.h"
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

    // TODO: Remove this
    FFATAL("A test message: %f", 3.14f);
    FERROR("A test message: %f", 3.14f);
    FWARN("A test message: %f", 3.14f);
    FINFO("A test message: %f", 3.14f);
    FDEBUG("A test message: %f", 3.14f);
    FTRACE("A test message: %f", 3.14f);

    app_state.is_running = TRUE;
    app_state.is_suspended = FALSE;

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
        }
    }

    app_state.is_running = FALSE;
    platform_shutdown(&app_state.platform);

    return TRUE;
}