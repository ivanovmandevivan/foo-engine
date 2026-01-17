#pragma once

#include <defines.h>
#include <game_types.h>

typedef struct game_state
{
    float32_t delta_time;
} game_state;

bool8_t game_initialize(game* game_instance);

bool8_t game_update(game* game_instance, float32_t delta_time);

bool8_t game_render(game* game_instance, float32_t delta_time);

void game_on_resize(game* game_instance, uint32_t width, uint32_t height);