#include "arch/mir_arch.h"
#include "mir_types.h"
#include "mir_utils.h"

void config_gatti(const char* conf_str)
{/*{{{*/
    return;
}/*}}}*/

void create_gatti()
{/*{{{*/
    return;
}/*}}}*/

void destroy_gatti()
{/*{{{*/
    return;
}/*}}}*/

uint16_t node_of_gatti(uint16_t coreid)
{/*{{{*/
    switch(coreid)
    {
        case 0:
        case 1:
        case 2:
        case 3:
            return 0;
            break;
        default:
            MIR_ABORT(MIR_ERROR_STR "Node of core not found!\n");
            break;
    }
    return 0;
}/*}}}*/

void cores_of_gatti(struct mir_sbuf_t* coreids, uint16_t nodeid)
{/*{{{*/
    MIR_ASSERT(coreids != NULL);
    coreids->size = 0;
    switch(nodeid)
    {
        case 0:
            coreids->size = 4;
            for(int i=0, basecore=0; i<4; i++)
                coreids->buf[i] = basecore+i;
            break;
        default:
            MIR_ABORT(MIR_ERROR_STR "Cores of node not found!\n");
            break;
    }
}/*}}}*/

uint16_t vicinity_of_gatti(uint16_t* neighbors, uint16_t nodeid, uint16_t diameter)
{/*{{{*/
    return 0;
}/*}}}*/

uint16_t comm_cost_of_gatti(uint16_t from_nodeid, uint16_t to_nodeid)
{/*{{{*/
    return 10;
}/*}}}*/

struct mir_arch_t arch_gatti = 
{
    .name = "gatti",
    .num_nodes = 1,
    .num_cores = 4,
    .diameter = 0,
    .llc_size_KB = 4096,
    .config = config_gatti,
    .create = create_gatti,
    .destroy = destroy_gatti,
    .node_of = node_of_gatti,
    .cores_of = cores_of_gatti,
    .vicinity_of = vicinity_of_gatti,
    .comm_cost_of = comm_cost_of_gatti
};

