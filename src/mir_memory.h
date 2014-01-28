#ifndef  MIR_MEMORY_H
#define  MIR_MEMORY_H 1

#include <stdlib.h>
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
void mir_page_attr_set(tmc_alloc_t* alloc);
#endif

END_C_DECLS 

#endif  

