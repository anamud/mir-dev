#include "mir_sched_pol.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

extern struct mir_sched_pol_t policy_central;
extern struct mir_sched_pol_t policy_ws;
extern struct mir_sched_pol_t policy_numa;

struct mir_sched_pol_t* mir_sched_pol_get_by_name(const char* name)
{
    if(0 == strcmp(name, "central"))
        return &policy_central;
    else if(0 == strcmp(name, "ws"))
        return &policy_ws;
    else if(0 == strcmp(name, "numa"))
        return &policy_numa;
    else
        return NULL;
}


