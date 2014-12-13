#include "arch/mir_arch.h"
#include "mir_types.h"
#include "mir_utils.h"

void config_tilepro64(const char* conf_str)
{/*{{{*/
    return;
}/*}}}*/

void create_tilepro64()
{/*{{{*/
    return;
}/*}}}*/

void destroy_tilepro64()
{/*{{{*/
    return;
}/*}}}*/

uint16_t sys_cpu_of_tilepro64(uint16_t cpuid)
{ return cpuid; }

uint16_t node_of_tilepro64(uint16_t cpuid)
{/*{{{*/
    if(cpuid > 63)
        MIR_ABORT(MIR_ERROR_STR "Node of CPU not found!\n");

    return cpuid;
}/*}}}*/

void cpus_of_tilepro64(struct mir_sbuf_t* cpuids, uint16_t nodeid)
{/*{{{*/
    MIR_ASSERT(cpuids != NULL);
    if(nodeid > 63)
        MIR_ABORT(MIR_ERROR_STR "CPU of node not found!\n");

    cpuids->size = 1;
    cpuids->buf[0] = nodeid;
}/*}}}*/

uint16_t vicinity_of_tilepro64(uint16_t* neighbors, uint16_t nodeid, uint16_t diameter)
{/*{{{*/
    return 0;
}/*}}}*/

uint16_t comm_cost_of_tilepro64(uint16_t from_nodeid, uint16_t to_nodeid)
{/*{{{*/
    uint8_t local_cost = 10;
    uint8_t remote_cost_base = 38;
    uint8_t remote_cost_per_hop = 2;

    if(from_nodeid == to_nodeid)
        return local_cost;

    uint16_t row_from = from_nodeid / 8;
    uint16_t col_from = from_nodeid % 8;
    uint16_t row_to = to_nodeid / 8;
    uint16_t col_to = to_nodeid % 8;

    uint16_t row_hop = abs(row_from - row_to);
    uint16_t col_hop = abs(col_from - col_to);

    return (remote_cost_base + remote_cost_per_hop * (row_hop + col_hop));
}/*}}}*/

struct mir_arch_t arch_tilepro64 = 
{/*{{{*/
    .name = "tilepro64",
    .num_nodes = 64,
    .num_cores = 64,
    .diameter = 14,
    .llc_size_KB = 64,
    .config = config_tilepro64,
    .create = create_tilepro64,
    .destroy = destroy_tilepro64,
    .node_of = node_of_tilepro64,
    .cpus_of = cpus_of_tilepro64,
    .vicinity_of = vicinity_of_tilepro64,
    .comm_cost_of = comm_cost_of_tilepro64
};/*}}}*/
