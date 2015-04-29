#include "scheduling/mir_sched_pol.h"
#include "mir_defines.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

extern struct mir_sched_pol_t policy_central;
extern struct mir_sched_pol_t policy_ws;
#ifdef MIR_MEM_POL_ENABLE
extern struct mir_sched_pol_t policy_numa;
#endif
extern struct mir_sched_pol_t policy_central_stack;
extern struct mir_sched_pol_t policy_ws_de;
extern struct mir_sched_pol_t policy_ws_de_node;

struct mir_sched_pol_t* mir_sched_pol_get_by_name(const char* name)
{/*{{{*/
    if(0 == strcmp(name, "central"))
        return &policy_central;
    else if(0 == strcmp(name, "ws"))
        return &policy_ws;
#ifdef MIR_MEM_POL_ENABLE
    else if(0 == strcmp(name, "numa"))
        return &policy_numa;
#endif
    else if(0 == strcmp(name, "central-stack"))
        return &policy_central_stack;
    else if(0 == strcmp(name, "ws-de"))
        return &policy_ws_de;
    else if(0 == strcmp(name, "ws-de-node"))
        return &policy_ws_de_node;
    else
        return NULL;
}/*}}}*/

