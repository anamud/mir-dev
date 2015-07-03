#include <ctype.h>
#include <errno.h>
#include <string.h>

#include "mir_loop.h"
#include "mir_memory.h"
#include "mir_omp_int.h"
#include "mir_runtime.h"
#include "mir_task.h"
#include "mir_utils.h"
#include "mir_worker.h"
#include "mir_lock.h"
#include "mir_recorder.h"
#include "mir_team.h"
#include "mir_barrier.h"

/* barrier.c */

void GOMP_barrier(void)
{ /*{{{*/
    struct mir_worker_t* worker = mir_worker_get_context();
    struct mir_omp_team_t* team;
    team = worker->current_task ? worker->current_task->team : NULL;
    if (team) {
        mir_barrier_wait(&team->barrier);
    }
} /*}}}*/

/* critical.c */

void GOMP_critical_start(void)
{ /*{{{*/
    mir_lock_set(&runtime->omp_critsec_lock);
} /*}}}*/

void GOMP_critical_end(void)
{ /*{{{*/
    mir_lock_unset(&runtime->omp_critsec_lock);
} /*}}}*/

/* loop.c, iter.c, env.c*/

static bool GOMP_loop_dynamic_next_int(long* pstart, long* pend)
{ /*{{{*/
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
    if (loop->incr < 0) {
        if (chunk < left)
            chunk = left;
    }
    else {
        if (chunk > left)
            chunk = left;
    }
    end = start + chunk;

    loop->next = end;

    *pstart = start;
    *pend = end;
    return true;
} /*}}}*/

bool GOMP_loop_dynamic_next(long* istart, long* iend)
{ /*{{{*/
    bool ret;

    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(worker != NULL);
    MIR_ASSERT(worker->current_task != NULL);
    MIR_ASSERT(worker->current_task->loop != NULL);

    struct mir_loop_des_t* loop = worker->current_task->loop;

    mir_lock_set(&(loop->lock));
    ret = GOMP_loop_dynamic_next_int(istart, iend);
    mir_lock_unset(&(loop->lock));

    return ret;
} /*}}}*/

void GOMP_parallel_loop_dynamic(void (*fn)(void*), void* data, unsigned num_threads, long start, long end, long incr, long chunk_size, unsigned flags)
{ /*{{{*/
    // Save loop description
    struct mir_loop_des_t* loop = mir_malloc_int(sizeof(struct mir_loop_des_t));
    MIR_ASSERT(loop != NULL);
    loop->incr = incr;
    loop->next = start;
    loop->end = ((incr > 0 && start > end) || (incr < 0 && start < end)) ? start : end;
    loop->chunk_size = chunk_size;
    loop->chunk_size *= incr;
    loop->static_trip = 0;
    mir_lock_create(&(loop->lock));

    // Create loop task on all workers
    mir_loop_task_create((mir_tfunc_t)fn, data, loop, 1, "GOMP_for_dynamic_task");

    // Wait for workers to finish
    mir_task_wait();
} /*}}}*/

bool GOMP_loop_runtime_start (long start, long end, long incr, long *istart, long *iend)
{
    MIR_ABORT("GOMP_loop_runtime_start not implemented");
}

bool GOMP_loop_dynamic_start (long start, long end, long incr, long chunk_size, long *istart, long *iend)
{
    MIR_ABORT("GOMP_loop_dynamic_start not implemented");
}

void GOMP_parallel_loop_runtime_start(void (*fn) (void *), void *data,
                                      unsigned num_threads, long start,
                                      long end, long incr)
{ /*{{{*/
    MIR_ABORT("GOMP_parallel_loop_runtime_start not implemented");
} /*}}}*/

static int GOMP_loop_static_next_int(long* pstart, long* pend)
{ /*{{{*/
    unsigned long nthreads = runtime->num_workers;

    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(worker != NULL);
    MIR_ASSERT(worker->current_task != NULL);
    MIR_ASSERT(worker->current_task->loop != NULL);

    struct mir_loop_des_t* loop = worker->current_task->loop;

    if (loop->static_trip == -1)
        return -1;

    /* We interpret chunk_size zero as "unspecified", which means that we
       should break up the iterations such that each thread makes only one
       trip through the outer loop.  */
    if (loop->chunk_size == 0) {
        unsigned long n, q, i, t;
        unsigned long s0, e0;
        long s, e;

        if (loop->static_trip > 0)
            return 1;

        /* Compute the total number of iterations.  */
        s = loop->incr + (loop->incr > 0 ? -1 : 1);
        n = (loop->end - loop->next + s) / loop->incr;
        i = worker->id;

        /* Compute the "zero-based" start and end points.  That is, as
           if the loop began at zero and incremented by one.  */
        q = n / nthreads;
        t = n % nthreads;
        if (i < t) {
            t = 0;
            q++;
        }
        s0 = q * i + t;
        e0 = s0 + q;

        /* Notice when no iterations allocated for this thread.  */
        if (s0 >= e0) {
            loop->static_trip = 1;
            return 1;
        }

        /* Transform these to the actual start and end numbers.  */
        s = (long)s0 * loop->incr + loop->next;
        e = (long)e0 * loop->incr + loop->next;

        *pstart = s;
        *pend = e;
        loop->static_trip = (e0 == n ? -1 : 1);
        return 0;
    }
    else {
        unsigned long n, s0, e0, i, c;
        long s, e;

        /* Otherwise, each thread gets exactly chunk_size iterations
           (if available) each time through the loop.  */

        s = loop->incr + (loop->incr > 0 ? -1 : 1);
        n = (loop->end - loop->next + s) / loop->incr;
        i = worker->id;
        c = loop->chunk_size;

        /* Initial guess is a C sized chunk positioned nthreads iterations
           in, offset by our thread number.  */
        s0 = (loop->static_trip * nthreads + i) * c;
        e0 = s0 + c;

        /* Detect overflow.  */
        if (s0 >= n)
            return 1;
        if (e0 > n)
            e0 = n;

        /* Transform these to the actual start and end numbers.  */
        s = (long)s0 * loop->incr + loop->next;
        e = (long)e0 * loop->incr + loop->next;

        *pstart = s;
        *pend = e;

        if (e0 == n)
            loop->static_trip = -1;
        else
            loop->static_trip++;
        return 0;
    }
} /*}}}*/

bool GOMP_loop_static_next(long* istart, long* iend)
{ /*{{{*/
    return !GOMP_loop_static_next_int(istart, iend);
} /*}}}*/

void GOMP_parallel_loop_static(void (*fn)(void*), void* data, unsigned num_threads, long start, long end, long incr, long chunk_size, unsigned flags)
{ /*{{{*/
    // Save loop description
    int num_workers = runtime->num_workers;
    struct mir_loop_des_t* loops = mir_malloc_int(num_workers * sizeof(struct mir_loop_des_t));
    MIR_ASSERT(loops != NULL);
    for (int i = 0; i < num_workers; ++i) {
        struct mir_loop_des_t* loop = &loops[i];
        loop->incr = incr;
        loop->next = start;
        loop->end = ((incr > 0 && start > end) || (incr < 0 && start < end)) ? start : end;
        loop->chunk_size = chunk_size;
        loop->static_trip = 0;
        mir_lock_create(&(loop->lock));
    }

    // Create loop task on all workers
    mir_loop_task_create((mir_tfunc_t)fn, data, loops, num_workers, "GOMP_for_static_task");

    // Wait for workers to finish
    mir_task_wait();
} /*}}}*/

void GOMP_parse_schedule(void)
{ /*{{{*/
    char* env, *end;
    unsigned long value;

    env = getenv("OMP_SCHEDULE");
    if (env == NULL)
        return;

    while (isspace(*env))
        ++env;
    if (strncasecmp(env, "static", 6) == 0) {
        runtime->omp_for_schedule = OFS_STATIC;
        env += 6;
    }
    else if (strncasecmp(env, "dynamic", 7) == 0) {
        runtime->omp_for_schedule = OFS_DYNAMIC;
        env += 7;
    }
    else
        goto unknown;

    while (isspace(*env))
        ++env;
    if (*env == '\0') {
        runtime->omp_for_chunk_size
            = runtime->omp_for_schedule != OFS_STATIC;
        return;
    }
    if (*env++ != ',')
        goto unknown;
    while (isspace(*env))
        ++env;
    if (*env == '\0')
        goto invalid;

    errno = 0;
    value = strtoul(env, &end, 10);
    if (errno)
        goto invalid;

    while (isspace(*end))
        ++end;
    if (*end != '\0')
        goto invalid;

    if ((int)value != value)
        goto invalid;

    if (value == 0 && runtime->omp_for_schedule != OFS_STATIC)
        value = 1;
    runtime->omp_for_chunk_size = value;
    return;

unknown:
    MIR_ABORT(MIR_ERROR_STR "Unknown value for OMP_SCHEDULE.\n");

invalid:
    MIR_ABORT(MIR_ERROR_STR "Invalid value for chunk size in OMP_SCHEDULE.\n");
} /*}}}*/

bool GOMP_loop_runtime_next(long* istart, long* iend)
{ /*{{{*/
    switch (runtime->omp_for_schedule) {
    case OFS_STATIC:
        return GOMP_loop_static_next(istart, iend);
    case OFS_DYNAMIC:
        return GOMP_loop_dynamic_next(istart, iend);
    case OFS_AUTO:
    case OFS_GUIDED:
    default:
        MIR_ABORT(MIR_ERROR_STR "OMP_SCHEDULE is unsupported.\n");
    }
} /*}}}*/

void GOMP_parallel_loop_runtime(void (*fn)(void*), void* data, unsigned num_threads, long start, long end, long incr, unsigned flags)
{ /*{{{*/
    // Create thread team.
    mir_create();

    switch (runtime->omp_for_schedule) {
    case OFS_STATIC:
        return GOMP_parallel_loop_static(fn, data, num_threads, start, end, incr, runtime->omp_for_chunk_size, flags);
    case OFS_DYNAMIC:
        return GOMP_parallel_loop_dynamic(fn, data, num_threads, start, end, incr, runtime->omp_for_chunk_size, flags);
    case OFS_AUTO:
    case OFS_GUIDED:
    default:
        MIR_ABORT(MIR_ERROR_STR "OMP_SCHEDULE is unsupported.\n");
    }
} /*}}}*/

void GOMP_loop_end(void)
{ /*{{{*/
    GOMP_barrier();
} /*}}}*/

void GOMP_loop_end_nowait(void)
{ /*{{{*/
    mir_soft_destroy();
} /*}}}*/

/* parallel.c */

void GOMP_parallel_start(void (*fn)(void*), void* data, unsigned num_threads)
{ /*{{{*/
    // Create thread team.
    mir_create();

    MIR_RECORDER_STATE_BEGIN(MIR_STATE_TCREATE);

    // Create task
    struct mir_worker_t* worker = mir_worker_get_context();

    // Workaround for our lack of proper OpenMP-handling of num_threads.
    num_threads = num_threads == 0 ? runtime->num_workers : num_threads;

    struct mir_omp_team_t* prevteam;
    prevteam = worker->current_task ? worker->current_task->team : NULL;
    struct mir_omp_team_t* team = mir_new_omp_team(prevteam, num_threads);
    struct mir_task_t* task = mir_task_create_common((mir_tfunc_t)fn, data, 0, 0, NULL, "GOMP_parallel_task", team);
    MIR_ASSERT(task != NULL);

    for (int i = 0; i < num_threads; i++) {
        if(i == worker->id)
            continue;
        mir_task_create_on_worker((mir_tfunc_t)fn, data, 0, 0, NULL, "GOMP_parallel_task", team, i);
    }

// Older GCCs force us to create a dummy task for the outline function
// which is called from the master thread. Newer GCCs force us to
// create an extra task that we schedule here.

#ifndef GCC_PRE_4_9
    mir_task_schedule_on_worker(task, -1);
#endif

    MIR_RECORDER_STATE_END(NULL, 0);

#ifdef GCC_PRE_4_9
    // Start profiling and book-keeping for parallel task
    mir_task_execute_prolog(task);
#endif
} /*}}}*/

void GOMP_parallel_end(void)
{ /*{{{*/
    struct mir_worker_t* worker = mir_worker_get_context();
    struct mir_omp_team_t* team;
    team = worker->current_task ? worker->current_task->team : NULL;

#ifndef GCC_PRE_4_9
    // Ensure the parallel block task is done.
    mir_task_wait();
#endif

    GOMP_barrier();

#ifdef GCC_PRE_4_9
    // Stop profiling and book-keeping for parallel task
    MIR_ASSERT(worker->current_task != NULL);
    mir_task_execute_epilog(worker->current_task);
#endif

    // Last task so unlink team.
    if (team != NULL)
        team->prev = NULL;

    mir_soft_destroy();
} /*}}}*/

void GOMP_parallel(void (*fn)(void*), void* data, unsigned num_threads, unsigned flags)
{ /*{{{*/
    GOMP_parallel_start(fn, data, num_threads);
    GOMP_parallel_end();
} /*}}}*/

/* task.c */

void GOMP_task(void (*fn)(void*), void* data, void (*copyfn)(void*, void*), long arg_size, long arg_align, bool if_clause, unsigned flags, void** depend)
{ /*{{{*/
    char task_name[MIR_SHORT_NAME_LEN];
    sprintf(task_name, "%p", &fn);

    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(worker != NULL);

    struct mir_omp_team_t* team;
    team = worker->current_task ? worker->current_task->team : NULL;

    if (copyfn) {
        char* buf = mir_malloc_int(sizeof(char) * arg_size);
        MIR_ASSERT(buf != NULL);
        copyfn(buf, data);
        mir_task_create_on_worker((mir_tfunc_t)fn, buf, (size_t)(arg_size), 0, NULL, task_name, team, -1);
    }
    else
        mir_task_create_on_worker((mir_tfunc_t)fn, data, (size_t)(arg_size), 0, NULL, task_name, team, -1);
} /*}}}*/

void GOMP_taskwait(void)
{ /*{{{*/
    mir_task_wait();
} /*}}}*/

/* single.c */

bool GOMP_single_start(void)
{ /*{{{*/
    struct mir_worker_t* worker = mir_worker_get_context();
    struct mir_omp_team_t* team;
    team = worker->current_task ? worker->current_task->team : NULL;

    if (team == NULL)
        return true;

    int sc = __sync_fetch_and_sub(&team->single_count, 1);
    if (sc == 1) {
        __sync_bool_compare_and_swap(&team->single_count, 0,
            team->num_threads);
    }
    return sc == team->num_threads;
} /*}}}*/

int omp_get_thread_num(void)
{ /*{{{*/
    if (runtime == NULL)
        return 0;
    struct mir_worker_t* worker = mir_worker_get_context();
    return worker->id;
} /*}}}*/

int omp_get_num_threads(void)
{ /*{{{*/
    return runtime ? runtime->num_workers : 1;
} /*}}}*/
