#include "scheduling/mir_sched_pol.h"
#include "mir_defines.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

struct mir_sched_pol_t* mir_sched_pol_get_by_name(const char* name)
{ /*{{{*/
    MIR_CONTEXT_ENTER;

    if (0 == strcmp(name, "central")) {
        MIR_CONTEXT_EXIT; return &policy_central;
    }
    else if (0 == strcmp(name, "ws")) {
        MIR_CONTEXT_EXIT; return &policy_ws;
    }
#ifdef MIR_MEM_POL_ENABLE
    else if (0 == strcmp(name, "numa")) {
        MIR_CONTEXT_EXIT; return &policy_numa;
    }
#endif
    else if (0 == strcmp(name, "central-stack")) {
        MIR_CONTEXT_EXIT; return &policy_central_stack;
    }
    else if (0 == strcmp(name, "ws-de")) {
        MIR_CONTEXT_EXIT; return &policy_ws_de;
    }
    else if (0 == strcmp(name, "ws-de-node")) {
        MIR_CONTEXT_EXIT; return &policy_ws_de_node;
    }
    else {
        MIR_CONTEXT_EXIT; return NULL;
    }

    MIR_CONTEXT_EXIT;
} /*}}}*/

