#include "mir_memory.h"
#include "mir_defines.h"
#include "mir_utils.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

uint64_t g_total_allocated_memory = 0;

// From bit-twiddling-hacks: http://graphics.stanford.edu/~seander/bithacks.html
static inline unsigned long upper_power_of_two(unsigned long v)
{ /*{{{*/
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
} /*}}}*/

void* mir_cmalloc_int(size_t bytes)
{ /*{{{*/
    void* memptr = mir_malloc_int(bytes);
    memset(memptr, 0, bytes);
    return memptr;
} /*}}}*/

uint64_t mir_get_allocated_memory()
{ /*{{{*/
    return g_total_allocated_memory;
} /*}}}*/

#ifdef __tile__

void* mir_malloc_int(size_t bytes)
{ /*{{{*/
    unsigned long bytes_p2 = upper_power_of_two((unsigned long)bytes);
    void* memptr = NULL;
    tmc_alloc_t alloc = TMC_ALLOC_INIT;
    mir_page_attr_set(&alloc);
    memptr = tmc_alloc_map(&alloc, bytes_p2);

#ifdef MIR_MEMORY_ALLOCATOR_DEBUG
    MIR_CHECK_MEM(memptr != NULL);
    __sync_fetch_and_add(&g_total_allocated_memory, bytes);
#endif

    return memptr;
} /*}}}*/

void mir_free_int(void* p, size_t bytes)
{ /*{{{*/
    MIR_ASSERT(p != NULL);
    MIR_ASSERT(bytes > 0);
    unsigned long bytes_p2 = upper_power_of_two((unsigned long)bytes);
    tmc_alloc_unmap(p, bytes_p2);
    p = NULL;
#ifdef MIR_MEMORY_ALLOCATOR_DEBUG
    __sync_fetch_and_sub(&g_total_allocated_memory, bytes);
#endif
} /*}}}*/

#else

void* mir_malloc_int(size_t bytes)
{ /*{{{*/
    unsigned long bytes_p2 = upper_power_of_two((unsigned long)bytes);
    void* memptr = NULL;
    int rval = posix_memalign(&memptr, MIR_PAGE_ALIGNMENT, bytes_p2);

#ifdef MIR_MEMORY_ALLOCATOR_DEBUG
    MIR_CHECK_MEM(rval == 0);
    __sync_fetch_and_add(&g_total_allocated_memory, bytes);
#endif

    return memptr;
} /*}}}*/

void mir_free_int(void* p, size_t bytes)
{ /*{{{*/
    MIR_ASSERT(p != NULL);
    MIR_ASSERT(bytes > 0);
    free(p);
    p = NULL;
#ifdef MIR_MEMORY_ALLOCATOR_DEBUG
    __sync_fetch_and_sub(&g_total_allocated_memory, bytes);
#endif
} /*}}}*/

#endif

