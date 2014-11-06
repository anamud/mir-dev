#include "arch/mir_arch.h"
#include "mir_types.h"
#include "mir_utils.h"

void config_gothmog(const char* conf_str)
{/*{{{*/
    return;
}/*}}}*/

void create_gothmog()
{/*{{{*/
    return;
}/*}}}*/

void destroy_gothmog()
{/*{{{*/
    return;
}/*}}}*/

uint16_t node_of_gothmog(uint16_t coreid)
{/*{{{*/
    switch(coreid)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            return 0;
            break;
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
            return 1;
            break;
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
            return 2;
            break;
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
            return 3;
            break;
        case 24:
        case 25:
        case 26:
        case 27:
        case 28:
        case 29:
            return 4;
            break;
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
            return 5;
            break;
        case 36:
        case 37:
        case 38:
        case 39:
        case 40:
        case 41:
            return 6;
            break;
        case 42:
        case 43:
        case 44:
        case 45:
        case 46:
        case 47:
            return 7;
            break;
        default:
            MIR_ABORT(MIR_ERROR_STR "Node of core not found!\n");
            break;
    }
}/*}}}*/

void cores_of_gothmog(struct mir_sbuf_t* coreids, uint16_t nodeid)
{/*{{{*/
    MIR_ASSERT(coreids != NULL);
    coreids->size = 0;
    switch(nodeid)
    {
        case 0:
            coreids->size = 6;
            for(int i=0, basecore=0; i<6; i++)
                coreids->buf[i] = basecore+i;
            break;
        case 1:
            coreids->size = 6;
            for(int i=0, basecore=6; i<6; i++)
                coreids->buf[i] = basecore+i;
            break;
        case 2:
            coreids->size = 6;
            for(int i=0, basecore=12; i<6; i++)
                coreids->buf[i] = basecore+i;
            break;
        case 3:
            coreids->size = 6;
            for(int i=0, basecore=18; i<6; i++)
                coreids->buf[i] = basecore+i;
            break;
        case 4:
            coreids->size = 6;
            for(int i=0, basecore=24; i<6; i++)
                coreids->buf[i] = basecore+i;
            break;
        case 5:
            coreids->size = 6;
            for(int i=0, basecore=30; i<6; i++)
                coreids->buf[i] = basecore+i;
            break;
        case 6:
            coreids->size = 6;
            for(int i=0, basecore=36; i<6; i++)
                coreids->buf[i] = basecore+i;
            break;
        case 7:
            coreids->size = 6;
            for(int i=0, basecore=42; i<6; i++)
                coreids->buf[i] = basecore+i;
            break;
        default:
            MIR_ABORT(MIR_ERROR_STR "Cores of node not found!\n");
            break;
    }
}/*}}}*/

uint16_t vicinity_of_gothmog(uint16_t* neighbors, uint16_t nodeid, uint16_t diameter)
{/*{{{*/
    uint16_t count = 0;
    if(diameter == 1)
    {
        switch(nodeid)
        {/*{{{*/
            case 0:
                count = 1;
                neighbors[0] = 1;
                break;
            case 1:
                count = 1;
                neighbors[0] = 0;
                break;
            case 2:
                count = 1;
                neighbors[0] = 3;
                break;
            case 3:
                count = 1;
                neighbors[0] = 2;
                break;
            case 4:
                count = 1;
                neighbors[0] = 5;
                break;
            case 5:
                count = 1;
                neighbors[0] = 4;
                break;
            case 6:
                count = 1;
                neighbors[0] = 7;
                break;
            case 7:
                count = 1;
                neighbors[0] = 6;
                break;
            default:
                break;
        }/*}}}*/
    }
    else if(diameter == 2)
    {
        switch(nodeid)
        {/*{{{*/
            case 0:
                count = 3;
                neighbors[0] = 2;
                neighbors[1] = 4;
                neighbors[2] = 6;
                break;
            case 2:
                count = 3;
                neighbors[0] = 0;
                neighbors[1] = 4;
                neighbors[2] = 6;
                break;
            case 4:
                count = 3;
                neighbors[0] = 0;
                neighbors[1] = 2;
                neighbors[2] = 6;
            case 6:
                count = 3;
                neighbors[0] = 0;
                neighbors[1] = 2;
                neighbors[2] = 4;
                break;
            case 1:
                count = 3;
                neighbors[0] = 3;
                neighbors[1] = 5;
                neighbors[2] = 7;
                break;
            case 3:
                count = 3;
                neighbors[0] = 1;
                neighbors[1] = 5;
                neighbors[2] = 7;
                break;
            case 5:
                count = 3;
                neighbors[0] = 1;
                neighbors[1] = 3;
                neighbors[2] = 7;
                break;
            case 7:
                count = 3;
                neighbors[0] = 1;
                neighbors[1] = 3;
                neighbors[2] = 5;
                break;
            default:
                break;
        }/*}}}*/
    }
    else if(diameter == 3)
    {
        switch(nodeid)
        {/*{{{*/
            case 0:
            case 2:
            case 4:
            case 6:
                count = 4;
                neighbors[0] = 1;
                neighbors[1] = 3;
                neighbors[2] = 5;
                neighbors[3] = 7;
                break;
            case 1:
            case 3:
            case 5:
            case 7:
                count = 4;
                neighbors[0] = 0;
                neighbors[1] = 2;
                neighbors[2] = 4;
                neighbors[3] = 6;
                break;
            default:
                break;
        }/*}}}*/
    }
    return count;
}/*}}}*/

/*static uint16_t socket_of(uint16_t nodeid)*/
/*{[>{{{<]*/
    /*switch(nodeid)*/
    /*{*/
        /*case 0:*/
        /*case 1:*/
            /*return 0;*/
            /*break;*/
        /*case 2:*/
        /*case 3:*/
            /*return 1;*/
            /*break;*/
        /*case 4:*/
        /*case 5:*/
            /*return 2;*/
            /*break;*/
        /*case 6:*/
        /*case 7:*/
            /*return 3;*/
            /*break;*/
        /*default:*/
            /*MIR_ABORT(MIR_ERROR_STR "Invalid socket query!");*/
    /*}*/
/*}[>}}}<]*/

/*uint16_t comm_cost_of_gothmog(uint16_t from_nodeid, uint16_t to_nodeid)*/
/*{[>{{{<]*/
    /*if(from_nodeid == to_nodeid)*/
        /*return 10;*/

    /*// Nodes in the same socket have a HT24 connection*/
    /*if(socket_of(from_nodeid) == socket_of(to_nodeid))*/
        /*return 16;*/

    /*// Even and odd-numbered nodes are connected together using HT8*/
    /*// Even-to-odd connection requires two hops over HT8*/
    /*if(from_nodeid % 2 != to_nodeid % 2)*/
        /*return 30;*/
    /*else*/
        /*return 20;*/
/*}[>}}}<]*/

uint16_t comm_cost_of_gothmog(uint16_t from_nodeid, uint16_t to_nodeid)
{/*{{{*/
    // These values straight from numactl --hardware
    if(from_nodeid == to_nodeid)
        return 10;

    // Even and odd-numbered nodes are connected together using HT8
    // Even-to-odd connection requires two hops over HT8
    if(from_nodeid % 2 != to_nodeid % 2)
        return 22;
    else
        return 16;
}/*}}}*/

struct mir_arch_t arch_gothmog = 
{/*{{{*/
    .name = "gothmog.it.kth.se",
    .num_nodes = 8,
    .num_cores = 48,
    .diameter = 3,
    .llc_size_KB = 5000,
    .config = config_gothmog,
    .create = create_gothmog,
    .destroy = destroy_gothmog,
    .node_of = node_of_gothmog,
    .cores_of = cores_of_gothmog,
    .vicinity_of = vicinity_of_gothmog,
    .comm_cost_of = comm_cost_of_gothmog
};/*}}}*/

