#include "game.h"

#include <core/logger.h>

bool8_t game_initialize(game* game_instance)
{
    FDEBUG("game_initialize() called!");
    return TRUE;
}

bool8_t game_update(game* game_instance, float32_t delta_time)
{
    return TRUE;
}

bool8_t game_render(game* game_instance, float32_t delta_time)
{
    return TRUE;
}

void game_on_resize(game* game_instance, uint32_t width, uint32_t height)
{

}