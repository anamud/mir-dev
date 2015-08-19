#!/usr/bin/python2

# NOTE: THIS SCRIPT REQUIRES PYTHON VERSION 2

import sys
import os
import csv

def write_pretext(fil):
    fil.write("""/* DO NOT EDIT. THIS FILE IS AUTO-GENERATED. */
#include "arch/mir_arch.h"
#include "mir_types.h"
#include "mir_defines.h"
#include "mir_utils.h"

void create_this()
{ return; }

void destroy_this()
{ return; }

uint16_t sys_cpu_of_this(uint16_t cpuid)
{ return cpuid; }

uint16_t node_of_this(uint16_t cpuid)
{ return 0; }

uint16_t vicinity_of_this(uint16_t* neighbors, uint16_t nodeid, uint16_t diameter)
{ return 0; }

uint16_t comm_cost_of_this(uint16_t from_nodeid, uint16_t to_nodeid)
{ return 10; }
""")

def write_posttext(fil):
    print 'Reading {0}/topology_this'.format(os.path.dirname(fil.name))
    with open('{0}/topology_this'.format(os.path.dirname(fil.name)), 'r') as f:
        reader = csv.reader(f)
        ctr = 0
        for row in reader:
            ctr = ctr + 1
        ctr = ctr - 1
    fil.write("""
void cpus_of_this(struct mir_sbuf_t* cpuids, uint16_t nodeid)
{{
    MIR_CONTEXT_ENTER;

    MIR_ASSERT(cpuids != NULL);
    cpuids->size = 0;
    if(nodeid != 0) {{
        MIR_CONTEXT_EXIT; return;
    }}
    else
    {{
        unsigned int num_cpus = {0};
        cpuids->size = num_cpus;
        for(int i=0; i<num_cpus; i++)
            cpuids->buf[i] = i;
    }}

    MIR_CONTEXT_EXIT;
}}
struct mir_arch_t arch_this = 
{{
    .name = "this",
    .num_nodes = 1,
    .num_cores = {1},
    .diameter = 0,
    .llc_size_KB = 1,
    .create = create_this,
    .destroy = destroy_this,
    .sys_cpu_of = sys_cpu_of_this,
    .node_of = node_of_this,
    .cpus_of = cpus_of_this,
    .vicinity_of = vicinity_of_this,
    .comm_cost_of = comm_cost_of_this
}};
""".format(ctr,ctr))

def main():
    if(len(sys.argv) > 2):
        print 'Usage: {0} path'.format(sys.argv[0])
        exit(2)
    fil = open('{0}/mir_arch_this.c'.format(sys.argv[1]), 'w')
    write_pretext(fil)
    write_posttext(fil)
    fil.close()
    print 'Wrote {0}/mir_arch_this.c'.format(sys.argv[1])

if __name__ == '__main__':
    main()
