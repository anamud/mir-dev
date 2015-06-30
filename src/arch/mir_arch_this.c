/* DO NOT EDIT. THIS FILE IS AUTO-GENERATED. */
#include "arch/mir_arch.h"
#include "mir_types.h"
#include "mir_utils.h"

void create_this()
{
    return;
}

void destroy_this()
{
    return;
}

uint16_t sys_cpu_of_this(uint16_t cpuid)
{
    return cpuid;
}

uint16_t node_of_this(uint16_t cpuid)
{
    return 0;
}

uint16_t vicinity_of_this(uint16_t* neighbors, uint16_t nodeid, uint16_t diameter)
{
    return 0;
}

uint16_t comm_cost_of_this(uint16_t from_nodeid, uint16_t to_nodeid)
{
    return 10;
}

void cpus_of_this(struct mir_sbuf_t* cpuids, uint16_t nodeid)
{
    MIR_ASSERT(cpuids != NULL);
    cpuids->size = 0;
    if (nodeid != 0)
        return;
    else {
        unsigned int num_cpus = 2;
        cpuids->size = num_cpus;
        for (int i = 0; i < num_cpus; i++)
            cpuids->buf[i] = i;
    }
}
struct mir_arch_t arch_this = {
    .name = "this",
    .num_nodes = 1,
    .num_cores = 2,
    .diameter = 0,
    .llc_size_KB = 1,
    .create = create_this,
    .destroy = destroy_this,
    .sys_cpu_of = sys_cpu_of_this,
    .node_of = node_of_this,
    .cpus_of = cpus_of_this,
    .vicinity_of = vicinity_of_this,
    .comm_cost_of = comm_cost_of_this
};
