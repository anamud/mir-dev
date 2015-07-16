#include "mir_memory.h"
#include "mir_loop.h"
#include "mir_utils.h"
#include "mir_lock.h"

struct mir_loop_des_t* mir_new_omp_loop_desc()
{ /*{{{*/
    struct mir_loop_des_t* loop = mir_malloc_int(sizeof(struct mir_loop_des_t));
    MIR_ASSERT(loop != NULL);
    mir_lock_create(&(loop->lock));
    loop->init = 0;
    return loop;
} /*}}}*/
