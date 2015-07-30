#include "mir_memory.h"
#include "mir_loop.h"
#include "mir_utils.h"
#include "mir_lock.h"

struct mir_loop_des_t* mir_new_omp_loop_desc()
{ /*{{{*/
    struct mir_loop_des_t* loop = mir_malloc_int(sizeof(struct mir_loop_des_t));
    MIR_CHECK_MEM(loop != NULL);
    mir_lock_create(&(loop->lock));
    loop->init = 0;
    return loop;
} /*}}}*/

void mir_omp_loop_desc_init(struct mir_loop_des_t* loop, long start, long end,
                            long incr, long chunk_size)
{ /*{{{*/
    loop->incr = incr;
    loop->next = start;
    loop->end = ((incr > 0 && start > end) || (incr < 0 && start < end)) ? start : end;
    loop->chunk_size = chunk_size;
    loop->static_trip = 0;
    loop->non_parallel_start = 0;
    loop->init = 1;
} /*}}}*/

struct mir_loop_des_t* mir_new_omp_loop_desc_init(long start, long end, long incr, long chunk_size)
{/*{{{*/
    struct mir_loop_des_t* loop = mir_new_omp_loop_desc();
    mir_omp_loop_desc_init(loop, start, end, incr, chunk_size);
    return loop;
}/*}}}*/
