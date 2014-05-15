#ifndef MIR_MEM_POL_H
#define MIR_MEM_POL_H

#include <stdint.h>
#include <stdlib.h>
#include "mir_types.h"

BEGIN_C_DECLS 

// Memory allocation related

void mir_mem_pol_config (const char* conf_str);

void mir_mem_pol_create ();

void mir_mem_pol_init ();

void mir_mem_pol_reset ();

void mir_mem_pol_destroy ();

void* mir_mem_pol_allocate (size_t sz);

void mir_mem_pol_release (void* addr, size_t sz);

// Memory node distribution related

struct mir_mem_node_dist_t
{/*{{{*/
    size_t* buf;
};/*}}}*/

struct mir_mem_node_dist_stat_t
{/*{{{*/
    size_t sum;
    size_t min;
    size_t max;
    double mean;
    double sd;
};/*}}}*/

struct mir_mem_node_dist_t* mir_mem_node_dist_create();

void  mir_mem_node_dist_destroy(struct mir_mem_node_dist_t* dist);

void mir_mem_get_dist(struct mir_mem_node_dist_t* dist, void* addr, size_t sz, void* part_of);

//size_t mir_mem_node_dist_sum(struct mir_mem_node_dist_t* dist);

void mir_mem_node_dist_stat(struct mir_mem_node_dist_t* dist, struct mir_mem_node_dist_stat_t* stat);

END_C_DECLS

#endif /* end of include guard: MIR_MEM_POL_H */

