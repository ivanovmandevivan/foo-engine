#pragma once

// Unsigned int typedefs:
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

// Signed int typedefs:
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;

// Floating point typedefs:
typedef float float32_t;
typedef double float64_t;

// Boolean typedefs:
typedef int bool32_t;
typedef char bool8_t;

// Define static assertion:
#if defined(__clang__) || defined(__gcc__)
#define STATIC_ASSERT _Static_assert
#else
#define STATIC_ASSERT static_assert
#endif

// Unsigned int static asserts:
STATIC_ASSERT(sizeof(uint8_t) == 1, "Expected uint8_t to be of size 1 byte.");
STATIC_ASSERT(sizeof(uint16_t) == 2, "Expected uint16_t to be of size 2 bytes.");
STATIC_ASSERT(sizeof(uint32_t) == 4, "Expected uint32_t to be of size 4 bytes.");
STATIC_ASSERT(sizeof(uint64_t) == 8, "Expected uint64_t to be of size 8 bytes.");

// Signed int static asserts:
STATIC_ASSERT(sizeof(int8_t) == 1, "Expected int8_t to be of size 1 byte.");
STATIC_ASSERT(sizeof(int16_t) == 2, "Expected int16_t to be of size 2 bytes.");
STATIC_ASSERT(sizeof(int32_t) == 4, "Expected int32_t to be of size 4 bytes.");
STATIC_ASSERT(sizeof(int64_t) == 8, "Expected int64_t to be of size 8 bytes.");

// Floating point static asserts:
STATIC_ASSERT(sizeof(float32_t) == 4, "Expected float32_t to be of size 4 bytes.");
STATIC_ASSERT(sizeof(float64_t) == 8, "Expected float64_t to be of size 8 bytes.");

// Boolean static asserts:
STATIC_ASSERT(sizeof(bool32_t) == 4, "Expected bool32_t to be of size 4 bytes.");
STATIC_ASSERT(sizeof(bool8_t) == 1, "Expected bool8_t to be of size 1 bytes.");

#define TRUE 1
#define FALSE 0

// Platform detection
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define KPLATFORM_WINDOWS 1
#ifndef _WIN64
#error "64-bit is required on Windows!"
#endif
#elif defined (__linux__) || defined(__gnu_linux__)
// Linux OS
#define KPLATFORM_LINUX 1
#if defined(__ANDROID__)
#define KPLATFORM_ANDROID 1
#endif
#elif defined(__unix__)
#define KPLATFORM_UNIX 1
#elif defined(_POSIX_VERSION)
// Posix
#define KPLATFORM_POSIX 1
#elif __APPLE__
// Apple platforms
#define KPLATFORM_APPLE 1
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR
// iOS Simulator
#define KPLATFORM_IOS 1
#define KPLATFORM_IOS_SIMULATOR 1
#elif TARGET_OS_IPHONE
#define KPLATFORM_IOS 1
// iOS device
#elif TARGET_OS_MAC
// Other kind of Mac OS
#else
#error "Unknown Apple platform"
#endif
#else
#error "Unknown platform!"
#endif

#ifdef KEXPORT
// Exports
#ifdef _MSC_VER
#define KAPI __declspec(dllexport)
#else
#define KAPI __attribute__((visibility("default")))
#endif
#else
// Imports
#ifdef _MSC_VER
#define KAPI __declspec(dllimport)
#else
#define KAPI
#endif
#endif