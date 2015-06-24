#ifndef  MIR_MEMORY_H
#define  MIR_MEMORY_H 1

#include <stdint.h>
#include "mir_types.h"

BEGIN_C_DECLS 

void* mir_malloc_int(size_t bytes);

void* mir_cmalloc_int(size_t bytes);

void mir_free_int(void *p, size_t bytes);

uint64_t mir_get_allocated_memory();

#ifdef __tile__
#include <tmc/alloc.h>
#include <tmc/mem.h>
static inline void page_attr_set(tmc_alloc_t* alloc)
{/*{{{*/
#if defined (MIR_PAGE_NO_LOCAL_CACHING)
    tmc_alloc_set_caching(alloc, MAP_CACHE_NO_LOCAL);
#elif defined (MIR_PAGE_NO_L1_CACHING)
    tmc_alloc_set_caching(alloc, MAP_CACHE_NO_L1);
#elif defined (MIR_PAGE_NO_L2_CACHING)
    tmc_alloc_set_caching(alloc, MAP_CACHE_NO_L2);
#endif
return;
}/*}}}*/
#endif

END_C_DECLS 

#endif  

