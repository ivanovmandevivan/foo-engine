#pragma once

#include <windows.h>

#include "core/application.h"
#include "core/logger.h"
#include "core/fmemory.h"
#include "game_types.h"

// Externally-defined function to create game:
extern bool8_t create_game(game* out_game);

/**
 * The main entry point of the application.
 *
 */
int main(void)
{
    initialize_memory();

    // Request the game instance from the application:
    game game_instance;
    if (!create_game(&game_instance))
    {
        FFATAL("Could not create game!");
        return -1;
    }

    // Ensure function ptrs exist:
    if (!game_instance.render || !game_instance.update || !game_instance.initialize || !game_instance.on_resize)
    {
        FFATAL("The game's function ptrs must be assigned!");
        return -2;
    }

    // Initialization:
    if (!application_create(&game_instance))
    {
        FINFO("Application fialed to create!\n");
        return 1;
    }

    // Begin the game loop:
    if (!application_run())
    {
        FINFO("Application did not shutdown gracefully.\n");
        return 2;
    }

    shutdown_memory();

    return 0;
}