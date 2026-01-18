#include "core/event.h"

#include "core/fmemory.h"
#include "containers/darray.h"

typedef struct registered_event
{
    void* listener;
    ptrfn_on_event callback;
} registered_event;

typedef struct event_code_entry
{
    registered_event* events;
} event_code_entry;

// This should be more than enough codes:
#define MAX_MESSAGE_CODES 16384

// State structure:
typedef struct event_system_state
{
    event_code_entry registered[MAX_MESSAGE_CODES];
} event_system_state;

/**
 * Event system internal state.
 */
static bool8_t is_initialized = FALSE;
static event_system_state state;

bool8_t event_initialize()
{
    if (is_initialized == TRUE)
    {
        return FALSE;
    }

    is_initialized = FALSE;
    fzero_memory(&state, sizeof(state));
    is_initialized = TRUE;
    return TRUE;
}

void event_shutdown()
{
    for (uint16_t i = 0; i < MAX_MESSAGE_CODES; ++i)
    {
        if (state.registered[i].events == 0)
        {
            continue;
        }

        darray_destroy(state.registered[i].events);
        state.registered[i].events = 0;
    }
}

bool8_t event_register(uint16_t code, void* listener, ptrfn_on_event on_event)
{
    if (is_initialized == FALSE)
    {
        return FALSE;
    }

    if (state.registered[code].events == 0)
    {
        state.registered[code].events = darray_create(registered_event);
    }

    uint64_t registered_count = darray_length(state.registered[code].events);
    for (uint64_t i = 0; i < registered_count; ++i)
    {
        if (state.registered[code].events[i].listener == listener)
        {
            // TODO: warn
            return FALSE;
        }
    }

    // If at this point, no duplicates were found, Proceed with registration:
    registered_event event;
    event.listener = listener;
    event.callback = on_event;
    darray_push(state.registered[code].events, event);
    return TRUE;
}

bool8_t event_unregister(uint16_t code, void* listener, ptrfn_on_event on_event)
{
    if (is_initialized == FALSE)
    {
        return FALSE;
    }

    // On nothing is registered for the code, early out.
    if (state.registered[code].events == 0)
    {
        // TODO: warn
        return FALSE;
    }

    uint64_t registered_count = darray_length(state.registered[code].events);
    for (uint64_t i = 0; i < registered_count; ++i)
    {
        registered_event e = state.registered[code].events[i];
        if (e.listener != listener && e.callback != on_event)
        {
            continue;
        }

        // TODO: Possible candidate to PopAndSwap given that order in events is irrelevant
        registered_event popped_event;
        darray_pop_at(state.registered[code].events, i, &popped_event);
        return TRUE;
    }

    return FALSE;
}

bool8_t event_fire(uint16_t code, void* sender, event_context context)
{
    if (is_initialized == FALSE)
    {
        return FALSE;
    }

    // If nothing is registered, early out:
    if (state.registered[code].events == 0)
    {
        // TODO: warn
        return FALSE;
    }

    uint64_t registered_count = darray_length(state.registered[code].events);
    for (uint64_t i = 0; i < registered_count; ++i)
    {
        registered_event e = state.registered[code].events[i];
        if (e.callback(code, sender, e.listener, context))
        {
            return TRUE;
        }
    }

    return FALSE;
}