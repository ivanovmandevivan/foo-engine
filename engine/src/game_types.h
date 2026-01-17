#pragma once

#include "core/application.h"

/*
 * Represents the basic game state inside the game.
 * Called for creation by the application.
 */
typedef struct game
{
    // The application config:
    application_config app_config;

    // Function ptr to game's initialize function:
    bool8_t (*initialize)(struct game* game_instance);

    // Function ptr to game's update function:
    bool8_t (*update)(struct game* game_instance, float32_t delta_time);

    // Function ptr to game's render function:
    bool8_t (*render)(struct game* game_instance, float32_t delta_time);

    // Function ptr to handle resizes, if applicable:
    void (*on_resize)(struct game* game_instance, uint32_t width, uint32_t height);

    // Game-specific game state. Created and managed by the game:
    void* state;
} game;