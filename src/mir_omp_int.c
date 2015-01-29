#include "mir_loop.h"
#include "mir_memory.h"
#include "mir_omp_int.h"
#include "mir_runtime.h"
#include "mir_task.h"
#include "mir_utils.h"
#include "mir_worker.h"
#include "mir_lock.h"

extern struct mir_runtime_t* runtime;

/* barrier.c */

void GOMP_barrier (void)
{
    MIR_DEBUG(MIR_DEBUG_STR "Note: GOMP_barrier is dummy.\n");
}

/* critical.c */

void GOMP_critical_start (void)
{
    mir_lock_set(&runtime->omp_critsec_lock);
}

void GOMP_critical_end (void)
{
    mir_lock_unset(&runtime->omp_critsec_lock);
}

/* loop.c */

bool GOMP_loop_dynamic_start (long start, long end, long incr, long chunk_size, long *istart, long *iend)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_dynamic_start not implemented yet!\n");
}

bool GOMP_loop_dynamic_next_locked (long *pstart, long *pend)
{
    struct mir_worker_t* worker = mir_worker_get_context(); 
    MIR_ASSERT(worker != NULL);
    MIR_ASSERT(worker->current_task != NULL);
    MIR_ASSERT(worker->current_task->loop != NULL);

    struct mir_loop_des_t* loop = worker->current_task->loop;

    long start, end, chunk, left;

    start = loop->next;
    if (start == loop->end)
        return false;

    chunk = loop->chunk_size;
    left = loop->end - start;
    if (loop->incr < 0)
    {
        if (chunk < left)
            chunk = left;
    }
    else
    {
        if (chunk > left)
            chunk = left;
    }
    end = start + chunk;

    loop->next = end;

    *pstart = start;
    *pend = end;
    return true;
}

bool GOMP_loop_dynamic_next (long *istart, long *iend)
{
    bool ret;

    struct mir_worker_t* worker = mir_worker_get_context(); 
    MIR_ASSERT(worker != NULL);
    MIR_ASSERT(worker->current_task != NULL);
    MIR_ASSERT(worker->current_task->loop != NULL);

    struct mir_loop_des_t* loop = worker->current_task->loop;

    mir_lock_set (&(loop->lock));
    ret = GOMP_loop_dynamic_next_locked (istart, iend);
    mir_lock_unset (&(loop->lock));

    return ret;
}

void GOMP_parallel_loop_dynamic_start (void (*fn) (void *), void *data, unsigned num_threads, long start, long end, long incr, long chunk_size)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_parallel_loop_dynamic_start not implemented yet!\n");
}

void GOMP_parallel_loop_dynamic (void (*fn) (void *), void *data, unsigned num_threads, long start, long end, long incr, long chunk_size, unsigned flags)
{
    MIR_DEBUG(MIR_DEBUG_STR "Note: GOMP_parallel_loop_dynamic does not create worker threads. Create a team of threads by calling GOMP_parallel prior.\n");

    // Save loop description
    struct mir_loop_des_t* loop = mir_malloc_int(sizeof(struct mir_loop_des_t));
    MIR_ASSERT(loop != NULL);
    loop->incr = incr;
    loop->next = start;
    loop->end = ((incr > 0 && start > end) || (incr < 0 && start < end)) ? start : end;
    loop->chunk_size = chunk_size;
    loop->chunk_size *= incr;
    mir_lock_create(&(loop->lock)); 

    // Create loop task on all workers
    mir_loop_task_create((mir_tfunc_t) fn, (void*) data, loop, "GOMP_for_dynamic_task");

    // Wait for workers to finish
    mir_task_wait();
}

void GOMP_loop_end (void)
{
    MIR_DEBUG(MIR_DEBUG_STR "Note: GOMP_loop_end is dummy.\n");
    return;
}

void GOMP_loop_end_nowait (void)
{
    MIR_DEBUG(MIR_DEBUG_STR "Note: GOMP_loop_end_nowait is dummy.\n");
    return;
}

/* parallel.c */

void GOMP_parallel_start (void (*fn) (void *), void * data, unsigned num_threads)
{
    MIR_DEBUG(MIR_DEBUG_STR "Note: GOMP_parallel_start implementation ignores num_threads argument. Use MIR_CONF to set number of threads.\n");
    MIR_DEBUG(MIR_DEBUG_STR "Note: GOMP_parallel_start executes the parallel block only on worker 0.\n");
    mir_create();
    mir_task_create_on_worker((mir_tfunc_t) fn, (void*) data, (size_t)(0), 0, NULL, "GOMP_parallel_task", 0);
}

void GOMP_parallel_end (void)
{
    mir_task_wait();
    mir_destroy();
}

void GOMP_parallel (void (*fn) (void *), void *data, unsigned num_threads, unsigned flags)
{
    GOMP_parallel_start(fn, data, num_threads);
    GOMP_parallel_end();
}

/* task.c */

void GOMP_task (void (*fn) (void *), void *data, void (*copyfn) (void *, void *), long arg_size, long arg_align, bool if_clause, unsigned flags, void **depend)
{  
    if(copyfn)
    {
        char* buf = mir_malloc_int(sizeof(char) * arg_size);
        MIR_ASSERT(buf != NULL);
        copyfn(buf, data); 
        mir_task_create((mir_tfunc_t) fn, (void*) buf, (size_t)(arg_size), 0, NULL, NULL);
    }
    else
        mir_task_create((mir_tfunc_t) fn, (void*) data, (size_t)(arg_size), 0, NULL, NULL);
}

void GOMP_taskwait (void)
{
    mir_task_wait();
}

/* single.c */

bool GOMP_single_start (void)
{
    MIR_DEBUG(MIR_DEBUG_STR "Note: GOMP_single_start implementation always returns true.\n");
    return true;
}
