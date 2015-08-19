#include "arch/mir_arch.h"
#include "mir_types.h"
#include "mir_defines.h"
#include "mir_utils.h"

void create_firenze()
{ /*{{{*/
    return;
} /*}}}*/

void destroy_firenze()
{ /*{{{*/
    return;
} /*}}}*/

uint16_t sys_cpu_of_firenze(uint16_t cpuid)
{ /*{{{*/
    MIR_CONTEXT_ENTER;

    struct mir_arch_topology_t topology_firenze[] = {
        { 0, 0, 0, 0, 0 },
        { 1, 2, 1, 0, 0 }
    };
    MIR_ASSERT(topology_firenze[cpuid].log_cpu == cpuid);

    MIR_CONTEXT_EXIT; return topology_firenze[cpuid].sys_cpu;
} /*}}}*/

uint16_t node_of_firenze(uint16_t cpuid)
{ /*{{{*/
    MIR_CONTEXT_ENTER;

    switch (cpuid) {
    case 0:
    case 1:
        MIR_CONTEXT_EXIT; return 0;
        break;
    default:
        MIR_LOG_ERR("Node of CPU %d not found.", cpuid);
        break;
    }

    MIR_CONTEXT_EXIT; return 0;
} /*}}}*/

void cpus_of_firenze(struct mir_sbuf_t* cpuids, uint16_t nodeid)
{ /*{{{*/
    MIR_CONTEXT_ENTER;

    MIR_ASSERT(cpuids != NULL);
    cpuids->size = 0;
    switch (nodeid) {
    case 0:
        cpuids->size = 2;
        for (int i = 0, basecpu = 0; i < 2; i++)
            cpuids->buf[i] = basecpu + i;
        break;
    default:
        MIR_LOG_ERR("CPUs of node %d not found.", nodeid);
        break;
    }

    MIR_CONTEXT_EXIT;
} /*}}}*/

uint16_t vicinity_of_firenze(uint16_t* neighbors, uint16_t nodeid, uint16_t diameter)
{ /*{{{*/
    return 0;
} /*}}}*/

uint16_t comm_cost_of_firenze(uint16_t from_nodeid, uint16_t to_nodeid)
{ /*{{{*/
    return 10;
} /*}}}*/

struct mir_arch_t arch_firenze = {
    .name = "firenze",
    .num_nodes = 1,
    .num_cores = 2,
    .diameter = 0,
    .llc_size_KB = 3072,
    .create = create_firenze,
    .destroy = destroy_firenze,
    .sys_cpu_of = sys_cpu_of_firenze,
    .node_of = node_of_firenze,
    .cpus_of = cpus_of_firenze,
    .vicinity_of = vicinity_of_firenze,
    .comm_cost_of = comm_cost_of_firenze
};

