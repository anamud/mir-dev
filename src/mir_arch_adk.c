#include "mir_arch.h"
#include "mir_types.h"
#include "mir_debug.h"

void config_adk(const char* conf_str)
{/*{{{*/
    return;
}/*}}}*/

void create_adk()
{/*{{{*/
    return;
}/*}}}*/

void destroy_adk()
{/*{{{*/
    return;
}/*}}}*/

uint16_t node_of_adk(uint16_t coreid)
{/*{{{*/
    switch(coreid)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            return 0;
            break;
        default:
            MIR_ABORT(MIR_ERROR_STR "Node of core not found!\n");
            break;
    }
    return 0;
}/*}}}*/

void cores_of_adk(uint16_t nodeid, struct mir_sbuf_t* coreids)
{/*{{{*/
    coreids->size = 0;
    switch(nodeid)
    {
        case 0:
            coreids->size = 8;
            for(int i=0, basecore=0; i<8; i++)
                coreids->buf[i] = basecore+i;
            break;
        default:
            MIR_ABORT(MIR_ERROR_STR "Cores of node not found!\n");
            break;
    }
}/*}}}*/

void vicinity_of_adk(uint16_t coreid, struct mir_sbuf_t* coreids)
{/*{{{*/
    MIR_ABORT(MIR_ERROR_STR "Vicinity information not added yet!\n");
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
    .config = config_adk,
    .create = create_adk,
    .destroy = destroy_adk,
    .node_of = node_of_adk,
    .cores_of = cores_of_adk,
    .vicinity_of = vicinity_of_adk,
    .comm_cost_of = comm_cost_of_adk
};

