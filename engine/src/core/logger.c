#include "logger.h"
#include "asserts.h"

// TODO: Temp, remove later.
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

const int32_t KMESSAGE_SIZE = 32000;

bool8_t initialize_logging()
{
    // TODO: create log file.
    return TRUE;
}

void shutdown_logging()
{
    // TODO: cleanup logging/write queued entries.
}

void log_output(log_level level, const char* message, ...)
{
    const char* level_strings[6] = {"[FATAL]: ", "[ERROR]: ", "[WARN]: ", "[INFO]: ", "[DEBUG]: ", "[TRACE]: "};
    //bool8_t is_error = level < 2;

    // Technically imposes a 32k character limit on a single log entry, but...
    // Have to change this and address this later (this is an effort to avoid dynamic mem, for now...)
    char out_message[KMESSAGE_SIZE] = {0};

    // Format original message.
    __builtin_va_list arg_ptr;
    va_start(arg_ptr, message);
    vsnprintf(out_message, KMESSAGE_SIZE, message, arg_ptr);
    va_end(arg_ptr);

    // TODO: platform-specific output.
    printf("%s%s\n", level_strings[level], out_message);
}

void report_assertion_failure(const char* expression, const char* message, const char* file, int32_t line)
{
    log_output(LOG_LEVEL_FATAL, "Assertion Failure: %s, message: '%s', in file: %s, line: %d\n", expression,
        message, file, line);
}