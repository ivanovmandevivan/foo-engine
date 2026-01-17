cmake_minimum_required(VERSION 3.15)
project(FooEngine C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_COMPILER "clang")

# -- Dependencies ---
find_package(Vulkan REQUIRED)

# -- Engine Library (Foo) ---
file(GLOB_RECURSE ENGINE_SOURCES "engine/src/*.c")

add_library(foo SHARED ${ENGINE_SOURCES}
        engine/src/core/logger.h
        engine/src/core/logger.c
        engine/src/core/asserts.h
        engine/src/platform/platform.h
        engine/src/platform/platform_win32.c
        engine/src/platform/platform_linux.c)

# Include directories
target_include_directories(foo PUBLIC "engine/src")

# Defines (Pre-processor)
# KEXPORT is required to export symbols from the DLL
# _DEBUG is often used for debug builds
# _CRT_SECURE_NO_WARNINGS silences MSVC warnings
target_compile_definitions(foo PRIVATE
    KEXPORT
    _DEBUG
    _CRT_SECURE_NO_WARNINGS
)

# Link Libraries
target_link_libraries(foo Vulkan::Vulkan)

# --- Testbed Executable ---
# Gather all source files in testbed/src
file(GLOB_RECURSE TESTBED_SOURCES "testbed/src/*.c")

add_executable(testbed ${TESTBED_SOURCES})

# Include directories (needs engine headers)
target_include_directories(testbed PRIVATE "testbed/src")

# Defines
# KIMPORT is required to import symbols from the DLLs
target_compile_definitions(testbed PRIVATE
    KIMPORT
    _DEBUG
    _CRT_SECURE_NO_WARNINGS
)

# Link against the engine library
target_link_libraries(testbed foo)

# -- Post-Build (Copy DLL) ---
# Ensure the compiled foo.dll is in the same folder as testbed.exe
add_custom_command(TARGET testbed POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:foo>
    $<TARGET_FILE_DIR:testbed>
)