#include "scheduling/mir_sched_pol.h"
#ifdef MIR_MEM_POL_ENABLE
#include "mir_mem_pol.h"
#endif
#include "mir_runtime.h"
#include "arch/mir_arch.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

extern struct mir_sched_pol_t policy_central;
extern struct mir_sched_pol_t policy_ws;
#ifdef MIR_MEM_POL_ENABLE
extern struct mir_sched_pol_t policy_numa;
#endif
extern struct mir_sched_pol_t policy_central_stack;
extern struct mir_sched_pol_t policy_ws_de;

extern struct mir_runtime_t* runtime;

struct mir_sched_pol_t* mir_sched_pol_get_by_name(const char* name)
{/*{{{*/
    if(0 == strcmp(name, "central"))
        return &policy_central;
    else if(0 == strcmp(name, "ws"))
        return &policy_ws;
#ifdef MIR_MEM_POL_ENABLE
    else if(0 == strcmp(name, "numa"))
        return &policy_numa;
#endif
    else if(0 == strcmp(name, "central-stack"))
        return &policy_central_stack;
    else if(0 == strcmp(name, "ws-de"))
        return &policy_ws_de;
    else
        return NULL;
}/*}}}*/

#ifdef MIR_MEM_POL_ENABLE
unsigned long mir_sched_pol_get_comm_cost(uint16_t node, struct mir_mem_node_dist_t* dist)
{/*{{{*/
    unsigned long comm_cost = 0;

    for(int i=0; i<runtime->arch->num_nodes; i++)
    {
        //MIR_DEBUG(MIR_DEBUG_STR "Comm cost node %d to %d: %d\n", node, i, runtime->arch->comm_cost_of(node, i));
        comm_cost += (dist->buf[i] * runtime->arch->comm_cost_of(node, i));
    }

    return comm_cost;
}/*}}}*/
#endif



