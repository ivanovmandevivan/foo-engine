#pragma once

#include "defines.h"

typedef struct clock
{
    float64_t start_time;
    float64_t elapsed;
} clock;

// Updates the provided clock. Should be called just before checking elapsed time.
// Has no effect on non-started clocks:
void clock_update(clock* clock);

// Starts the provided clock. Resets elapsed time:
void clock_start(clock* clock);

// Stops the provided clock. Does not reset elapsed time:
void clock_stop(clock* clock);