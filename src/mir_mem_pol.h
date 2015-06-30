#ifndef MIR_MEM_POL_H
#define MIR_MEM_POL_H

#include <stdint.h>
#include "mir_types.h"
#include "mir_defines.h"

BEGIN_C_DECLS
#ifdef MIR_MEM_POL_ENABLE

// Memory allocation related

void mir_mem_pol_config(const char* conf_str);

void mir_mem_pol_create();

void mir_mem_pol_init();

void mir_mem_pol_destroy();

/*LIBINT*/ void mir_mem_pol_reset();

/*LIBINT*/ void* mir_mem_pol_allocate(size_t sz);

/*LIBINT*/ void mir_mem_pol_release(void* addr, size_t sz);

// Memory node distribution related

struct mir_mem_node_dist_t { /*{{{*/
    size_t* buf;
}; /*}}}*/

struct mir_mem_node_dist_stat_t { /*{{{*/
    size_t sum;
    size_t min;
    size_t max;
    double mean;
    double sd;
}; /*}}}*/

struct mir_mem_node_dist_t* mir_mem_node_dist_create();

void mir_mem_node_dist_destroy(struct mir_mem_node_dist_t* dist);

void mir_mem_get_mem_node_dist(struct mir_mem_node_dist_t* dist, void* addr, size_t sz, void* part_of);

void mir_mem_node_dist_get_stat(struct mir_mem_node_dist_stat_t* stat, const struct mir_mem_node_dist_t* dist);

unsigned long mir_mem_node_dist_get_comm_cost(const struct mir_mem_node_dist_t* dist, uint16_t from_node);
#else
#define mir_mem_pol_create() ;
#define mir_mem_pol_destroy() ;
#define mir_mem_pol_init() ;
#define mir_mem_pol_config(x) ;
#endif
END_C_DECLS

#endif /* end of include guard: MIR_MEM_POL_H */
