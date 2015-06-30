#ifndef MIR_ARCH_H
#define MIR_ARCH_H 1

#include "stdint.h"
#include "mir_types.h"

BEGIN_C_DECLS

struct mir_arch_topology_t {
    uint16_t log_cpu;
    uint16_t sys_cpu;
    uint16_t core;
    uint16_t socket;
    uint16_t node;
};

struct mir_arch_t { /*{{{*/
    // Constants
    const char* name;
    uint16_t num_cores;
    uint16_t num_nodes;
    uint16_t diameter; // Largest hop distance between two nodes
    size_t llc_size_KB;

    // Interfaces
    void (*config)(const char* conf_str);
    void (*create)();
    void (*destroy)();
    uint16_t (*sys_cpu_of)(uint16_t cpuid);
    uint16_t (*node_of)(uint16_t cpuid);
    void (*cpus_of)(struct mir_sbuf_t* cpuids, uint16_t nodeid);
    uint16_t (*vicinity_of)(uint16_t* neighbors, uint16_t nodeid, uint16_t diameter);
    uint16_t (*comm_cost_of)(uint16_t from_nodeid, uint16_t to_nodeid);
}; /*}}}*/

struct mir_arch_t* mir_arch_create_by_query();

END_C_DECLS

#endif //MIR_ARCH_H 1
