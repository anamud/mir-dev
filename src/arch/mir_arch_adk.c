#include "arch/mir_arch.h"
#include "mir_types.h"
#include "mir_utils.h"

void create_adk()
{/*{{{*/
    return;
}/*}}}*/

void destroy_adk()
{/*{{{*/
    return;
}/*}}}*/

uint16_t sys_cpu_of_adk(uint16_t cpuid)
{ 
    struct mir_arch_topology_t topology_adk[] = {
        {0,0,0,0,0},
        {1,2,1,0,0},
        {2,3,2,0,0},
        {3,6,3,0,0}
    };
    MIR_ASSERT(topology_adk[cpuid].log_cpu == cpuid);
    return topology_adk[cpuid].sys_cpu;
}

uint16_t node_of_adk(uint16_t cpuid)
{/*{{{*/
    switch(cpuid)
    {
        case 0:
        case 1:
        case 2:
        case 3:
            return 0;
            break;
        default:
            MIR_ABORT(MIR_ERROR_STR "Node of CPU not found!\n");
            break;
    }
    return 0;
}/*}}}*/

void cpus_of_adk(struct mir_sbuf_t* cpuids, uint16_t nodeid)
{/*{{{*/
    MIR_ASSERT(cpuids != NULL);
    cpuids->size = 0;
    switch(nodeid)
    {
        case 0:
            cpuids->size = 4;
            for(int i=0, basecpu=0; i<4; i++)
                cpuids->buf[i] = basecpu+i;
            break;
        default:
            MIR_ABORT(MIR_ERROR_STR "CPUs of node not found!\n");
            break;
    }
}/*}}}*/

uint16_t vicinity_of_adk(uint16_t* neighbors, uint16_t nodeid, uint16_t diameter)
{/*{{{*/
    return 0;
}/*}}}*/

uint16_t comm_cost_of_adk(uint16_t from_nodeid, uint16_t to_nodeid)
{/*{{{*/
    return 10;
}/*}}}*/

struct mir_arch_t arch_adk = 
{
    .name = "adk",
    .num_nodes = 1,
    .num_cores = 4,
    .diameter = 0,
    .llc_size_KB = 4096,
    .create = create_adk,
    .destroy = destroy_adk,
    .sys_cpu_of = sys_cpu_of_adk,
    .node_of = node_of_adk,
    .cpus_of = cpus_of_adk,
    .vicinity_of = vicinity_of_adk,
    .comm_cost_of = comm_cost_of_adk
};

