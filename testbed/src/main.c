#include <core/logger.h>
#include <core/asserts.h>

// TODO: test
#include <platform/platform.h>

int main(void)
{
    FFATAL("A test message: %f", 3.14f);
    FERROR("A test message: %f", 3.14f);
    FWARN("A test message: %f", 3.14f);
    FINFO("A test message: %f", 3.14f);
    FDEBUG("A test message: %f", 3.14f);
    FTRACE("A test message: %f", 3.14f);

    platform_state state;
    if (platform_startup(&state, "Foo Engine Testbed", 100, 100, 1280, 720))
    {
        while (TRUE)
        {
            platform_pump_messages(&state);
        }
    }

    platform_shutdown(&state);
    return 0;
}