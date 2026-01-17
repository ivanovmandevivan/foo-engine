#pragma once

#include "defines.h"

// Disable assertions by commenting out the below line:
#define FASSERTIONS_ENABLED

#ifdef FASSERTIONS_ENABLED
#if defined(_MSC_VER)
#include <intrin.h>
#define debugBreak() __debugbreak()
#elif defined(__APPLE__) || defined(__linux__)
#include <signal.h>
#define debugBreak() raise(SIGTRAP)
#else
#define debugBreak() __builtin_trap()
#endif

FAPI void report_assertion_failure(const char* expression, const char* message, const char* file, int32_t line);

#define FASSERT(expr)                                                \
do {                                                                 \
   if (!(expr))                                                      \
   {                                                                 \
      report_assertion_failure(#expr, "", __FILE__, __LINE__);       \
      debugBreak();                                                  \
   }                                                                 \
} while (0)

#define FASSERT_MSG(expr, message)                                   \
do {                                                                 \
   if (!(expr))                                                      \
   {                                                                 \
      report_assertion_failure(#expr, message, __FILE__, __LINE__);  \
      debugBreak();                                                  \
   }                                                                 \
} while (0)                                                          \

#ifdef _DEBUG
#define FASSERT_DEBUG(expr) FASSERT(expr)
#else
#define FASSERT_DEBUG(expr)
#endif

#else
#define FASSERT(expr)
#define FASSERT_MSG(expr, message)
#define FASSERT_DEBUG(expr)
#endif