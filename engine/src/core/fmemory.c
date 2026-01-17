#include "fmemory.h"

// TODO: Custom string library.
#include <string.h>
#include <stdio.h>

#include "core/logger.h"
#include "platform/platform.h"

struct memory_stats
{
    uint64_t total_allocated;
    uint64_t tagged_allocations[MEMORY_TAG_MAX_TAGS];
};

static const char* memory_tag_strings[MEMORY_TAG_MAX_TAGS] =
{
    "UNKNOWN    ",
    "ARRAY      ",
    "DARRAY     ",
    "DICT       ",
    "RING_QUEUE ",
    "BST        ",
    "STRING     ",
    "APPLICATION",
    "JOB        ",
    "TEXTURE    ",
    "MAT_INST   ",
    "RENDERER   ",
    "GAME       ",
    "TRANSFORM  ",
    "ENTITY     ",
    "ENTITY_NODE",
    "SCENE      "
};

static struct memory_stats stats;

void initialize_memory()
{
    platform_zero_memory(&stats, sizeof(stats));
}

void shutdown_memory()
{
    // TODO: For now this won't do anything, but will have functionality later.
}

void* fallocate(uint64_t size, memory_tag tag)
{
    if (tag == MEMORY_TAG_UNKNOWN)
    {
        FWARN("fallocate called using MEMORY_TAG_UNKNOWN. Re-class this allocation.");
    }

    stats.total_allocated += size;
    stats.tagged_allocations[tag] += size;

    // TODO: Will deal with mem alignment later.
    void* block = platform_allocate(size, FALSE);
    platform_zero_memory(block, size);
    return block;
}

void ffree(void* block, uint64_t size, memory_tag tag)
{
    if (tag == MEMORY_TAG_UNKNOWN)
    {
        FWARN("ffree called using MEMORY_TAG_UNKNOWN. Re-class this allocation.");
    }

    stats.total_allocated -= size;
    stats.tagged_allocations[tag] -= size;

    // TODO: Will deal with mem alignment later.
    platform_free(block, FALSE);
}

void* fzero_memory(void* block, uint64_t size)
{
    return platform_zero_memory(block, size);
}

void* fcopy_memory(void* dest, const void* source, uint64_t size)
{
    return platform_copy_memory(dest, source, size);
}

void* fset_memory(void* dest, int32_t value, uint64_t size)
{
    return platform_set_memory(dest, value, size);
}

// TODO: this is a debug function and needs improving.
char* get_memory_usage_str()
{
    const uint64_t gib = 1024 * 1024 * 1024;
    const uint64_t mib = 1024 * 1024;
    const uint64_t kib = 1024;

    char buffer[8000] = "System memory use (tagged):\n";
    uint64_t offset = strlen(buffer);

    for (uint32_t i = 0; i < MEMORY_TAG_MAX_TAGS; ++i)
    {
        char unit[4] = "XiB";
        float amount = 1.0f;

        if (stats.tagged_allocations[i] >= gib)
        {
            unit[0] = 'G';
            amount = stats.tagged_allocations[i] / (float)gib;
        }
        else if (stats.tagged_allocations[i] >= mib)
        {
            unit[0] = 'M';
            amount = stats.tagged_allocations[i] / (float)mib;
        }
        else if (stats.tagged_allocations[i] >= kib)
        {
            unit[0] = 'K';
            amount = stats.tagged_allocations[i] / (float)kib;
        }
        else
        {
            unit[0] = 'B';
            unit[1] = 0;
            amount = (float)stats.tagged_allocations[i];
        }

        int32_t length = snprintf(buffer + offset, 8000, " %s: %.2f%s\n",
            memory_tag_strings[i], amount, unit);
        offset += length;
    }

    char* out_string = _strdup(buffer);
    return out_string;
}
