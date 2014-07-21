#!/usr/bin/python

# NOTE: THIS SCRIPT REQUIRES PYTHON VERSION > 3

import sys
import os
import multiprocessing

def write_pretext(fil):
    fil.write("""/* DO NOT EDIT. THIS FILE IS AUTO-GENERATED. */
#include "arch/mir_arch.h"
#include "mir_types.h"
#include "mir_utils.h"

void config_this(const char* conf_str)
{ return; }

void create_this()
{ return; }

void destroy_this()
{ return; }

uint16_t node_of_this(uint16_t coreid)
{ return 0; }

uint16_t vicinity_of_this(uint16_t* neighbors, uint16_t nodeid, uint16_t diameter)
{ return 0; }

uint16_t comm_cost_of_this(uint16_t from_nodeid, uint16_t to_nodeid)
{ return 10; }
""")

def write_posttext(fil):
    fil.write("""
void cores_of_this(struct mir_sbuf_t* coreids, uint16_t nodeid)
{{
    coreids->size = 0;
    if(nodeid != 0)
        return;
    else
    {{
        unsigned int num_cores = {};
        coreids->size = num_cores;
        for(int i=0, basecore=0; i<num_cores; i++)
            coreids->buf[i] = basecore+i;
    }}
}}
struct mir_arch_t arch_this = 
{{
    .name = "this",
    .num_nodes = 1,
    .num_cores = {},
    .diameter = 0,
    .llc_size_KB = 1,
    .config = config_this,
    .create = create_this,
    .destroy = destroy_this,
    .node_of = node_of_this,
    .cores_of = cores_of_this,
    .vicinity_of = vicinity_of_this,
    .comm_cost_of = comm_cost_of_this
}};
""".format(multiprocessing.cpu_count(),multiprocessing.cpu_count()))

def main():
    if sys.version_info < (3,0):
        print("Python version < 3. Aborting!")
        sys.exit(1)
    if(len(sys.argv) > 2):
        print('Usage: {} path'.format(sys.argv[0]))
        exit(2)
    fil = open('{}/mir_arch_this.c'.format(sys.argv[1]), 'w')
    write_pretext(fil)
    write_posttext(fil)
    fil.close()
    print('Wrote {}/mir_arch_this.c'.format(sys.argv[1]))

if __name__ == '__main__':
    main()
