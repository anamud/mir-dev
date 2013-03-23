#ifndef MIR_MEM_POL_H
#define MIR_MEM_POL_H

#include <stdint.h>
#include <stdlib.h>

// Memory allocation related

void mir_mem_pol_config (const char* conf_str);

void mir_mem_pol_create ();

void mir_mem_pol_destroy ();

void* mir_mem_pol_allocate (size_t sz);

void mir_mem_pol_release (void* addr, size_t sz);

// Memory node distribution related

struct mir_mem_node_dist_t
{/*{{{*/
    size_t* buf;
};/*}}}*/

struct mir_mem_node_dist_t* mir_mem_node_dist_create();

void  mir_mem_node_dist_destroy(struct mir_mem_node_dist_t* dist);

struct mir_mem_node_dist_t* mir_mem_get_dist(void* addr, size_t sz, void* part_of);
#endif /* end of include guard: MIR_MEM_POL_H */

