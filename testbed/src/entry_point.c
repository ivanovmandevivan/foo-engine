#include "game.h"
#include <entry_point.h>

// TODO: Remove this, we won't need it later, we dont want platform specific code here
#include <platform/platform.h>

bool8_t create_game(game* out_game)
{
    // Application configuration:
    out_game->app_config.start_pos_x = 100;
    out_game->app_config.start_pos_y = 100;
    out_game->app_config.start_width = 1280;
    out_game->app_config.start_height = 720;
    out_game->app_config.name = "Foo Engine Testbed";
    out_game->initialize = game_initialize;
    out_game->update = game_update;
    out_game->render = game_render;
    out_game->on_resize = game_on_resize;

    // Create the game state:
    out_game->state = platform_allocate(sizeof(game_state), FALSE);

    return TRUE;
}