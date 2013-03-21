#ifndef MIR_MEM_POL_H
#define MIR_MEM_POL_H

#include <stdint.h>
#include <stdlib.h>

void mir_mem_pol_config (const char* conf_str);

void mir_mem_pol_create ();

void mir_mem_pol_destroy ();

void* mir_mem_pol_allocate (size_t sz);

void mir_mem_pol_release (void* addr, size_t sz);
#endif /* end of include guard: MIR_MEM_POL_H */

