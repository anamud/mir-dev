#include "mir_twc.h"
#include "mir_memory.h"
#include "mir_utils.h"
#include "mir_runtime.h"

struct mir_twc_t* mir_twc_create()
{/*{{{*/
    struct mir_twc_t* twc = mir_malloc_int (sizeof(struct mir_twc_t));
    MIR_ASSERT(twc != NULL);

    // Book-keeping
    for(int i=0; i<runtime->num_workers; i++)
       twc->count_per_worker[i] = 0;
    twc->count = 0;

    // Reset num times passed
    twc->num_passes = 0;

    return twc;
}/*}}}*/
