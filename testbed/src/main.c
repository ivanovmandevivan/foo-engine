#include <core/logger.h>
#include <core/asserts.h>

#include <stdio.h>

int main(void)
{
    // This is a fix for CLion since it's the IDE I am using, avoids line buffering and prints immediately.
    // Disable buffering for standard output
    setvbuf(stdout, NULL, _IONBF, 0);
    // Disable buffering for standard error
    setvbuf(stderr, NULL, _IONBF, 0);

    FFATAL("A test message: %f", 3.14f);
    FERROR("A test message: %f", 3.14f);
    FWARN("A test message: %f", 3.14f);
    FINFO("A test message: %f", 3.14f);
    FDEBUG("A test message: %f", 3.14f);
    FTRACE("A test message: %f", 3.14f);

    FASSERT(1 == 0);
    return 0;
}