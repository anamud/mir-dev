#include "mir_sched_pol.h"
#include "mir_mem_pol.h"
#include "mir_runtime.h"
#include "mir_arch.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

extern struct mir_sched_pol_t policy_central;
extern struct mir_sched_pol_t policy_ws;
extern struct mir_sched_pol_t policy_numa;

extern struct mir_runtime_t* runtime;

struct mir_sched_pol_t* mir_sched_pol_get_by_name(const char* name)
{/*{{{*/
    if(0 == strcmp(name, "central"))
        return &policy_central;
    else if(0 == strcmp(name, "ws"))
        return &policy_ws;
    else if(0 == strcmp(name, "numa"))
        return &policy_numa;
    else
        return NULL;
}/*}}}*/

unsigned long get_comm_cost(uint16_t node, struct mir_mem_node_dist_t* dist)
{/*{{{*/
    unsigned long comm_cost = 0;

    for(int i=0; i<runtime->arch->num_nodes; i++)
        comm_cost += (dist->buf[i] * runtime->arch->comm_cost_of(node, i));

    return comm_cost;
}/*}}}*/



