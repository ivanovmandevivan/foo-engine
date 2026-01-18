#include "renderer_frontend.h"
#include "renderer_backend.h"

#include "core/logger.h"
#include "core/fmemory.h"

// Backend render context: (Constrained to only one backend, might want more in the future).
static renderer_backend* backend = 0;


bool8_t renderer_initialize(const char* application_name, struct platform_state* plat_state)
{
    backend = fallocate(sizeof(renderer_backend), MEMORY_TAG_RENDERER);

    // TODO: make this configurable:
    renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN, plat_state, backend);
    backend->frame_number = 0;

    if (!backend->initialize(backend, application_name, plat_state))
    {
        FFATAL("Renderer backend failed to initialize. Shutting down.");
        return FALSE;
    }

    return TRUE;
}

void renderer_shutdown()
{
    backend->shutdown(backend);
    ffree(backend, sizeof(renderer_backend), MEMORY_TAG_RENDERER);
}

void renderer_on_resize(uint16_t width, uint16_t height)
{

}

bool8_t renderer_begin_frame(float32_t delta_time)
{
    return backend->begin_frame(backend, delta_time);
}

bool8_t renderer_end_frame(float32_t delta_time)
{
    bool8_t result = backend->end_frame(backend, delta_time);
    backend->frame_number++;
    return result;
}

bool8_t renderer_draw_frame(render_packet* packet)
{
    // If the begin frame returned successfully, mid-frame operations may continue:
    if (renderer_begin_frame(packet->delta_time))
    {
        // End the frame. If this fails, it is likely unrecoverable:
        bool8_t result = renderer_end_frame(packet->delta_time);
        if (!result)
        {
            FERROR("renderer_end_frame failed. Application shutting down...");
            return FALSE;
        }
    }

    return TRUE;
}

