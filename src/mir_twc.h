#ifndef MIR_TWC_H
#define MIR_TWC_H 1

#include "mir_types.h"
#include "mir_defines.h"

BEGIN_C_DECLS

// The task wait counter
struct mir_twc_t
{/*{{{*/
    unsigned long count;
    unsigned long num_passes;
    struct mir_time_list_t* pass_time;
    struct mir_task_t* parent;
    unsigned int count_per_worker[MIR_WORKER_MAX_COUNT];
};/*}}}*/

struct mir_twc_t* mir_twc_create();

END_C_DECLS

#endif
