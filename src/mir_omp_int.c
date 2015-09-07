#include <sys/time.h>
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
#include "mir_defines.h"

/* MIR internal functions. */

static void parallel_start (void (*fn) (void *), void *data, unsigned num_threads, long start, long end, long incr, long chunk_size, bool has_loop_desc, bool private_loop_desc, const char* name)
{ /*{{{*/
    struct mir_loop_des_t* loop = NULL;
    if (has_loop_desc && !private_loop_desc) {
        loop = mir_new_omp_loop_desc_init(start, end, incr, chunk_size, false);
    }

    // Create thread team.
    mir_create_int(num_threads);

    // Ensure number of required threads is not larger than those available.
    MIR_ASSERT_STR(num_threads <= runtime->num_workers, "Number of OMP threads requested is greater than number of MIR workers.");

    // Create loop task on all workers.
    MIR_RECORDER_STATE_BEGIN(MIR_STATE_TCREATE);

    // Number of threads is either specified by the program or the number of threads created by the runtime system.
    num_threads = num_threads == 0 ? runtime->num_workers : num_threads;
    struct mir_worker_t* worker = mir_worker_get_context();
    struct mir_omp_team_t* prevteam;
    prevteam = worker->current_task ? worker->current_task->team : NULL;
    struct mir_omp_team_t* team = mir_new_omp_team(prevteam, num_threads);

    for (int i = 0; i < num_threads; i++) {
        if(i == worker->id || runtime->single_parallel_block)
            continue;

        // Set loop parameters.
        if (has_loop_desc && private_loop_desc) {
            loop = mir_new_omp_loop_desc_init(start, end, incr, chunk_size, false);
        }

        // Create and schedule loop tasks on all workers except current.
        mir_task_create_on_worker((mir_tfunc_t) fn, data, 0, 0, NULL,
           name, team, loop, i);
    }

    MIR_RECORDER_STATE_END(NULL, 0);

    // Set loop parameters for fake task.
    if (has_loop_desc && private_loop_desc) {
        loop = mir_new_omp_loop_desc_init(start, end, incr, chunk_size, false);
    }

    // Create fake loop task on current worker.
    struct mir_task_t* task = mir_task_create_common((mir_tfunc_t) fn, data,
           0, 0, NULL, name, team, loop, worker->current_task);
    MIR_CHECK_MEM(task != NULL);

    // Start profiling and book-keeping for fake task
    mir_task_execute_prolog(task);
    worker->current_task->team->parallel_block_flag[worker->id] = 1;
} /*}}}*/

static void chunk_task_start(const char* name, struct mir_loop_des_t* loop)
{/*{{{*/
    MIR_ASSERT(loop != NULL);
    MIR_ASSERT(name != NULL);

    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(worker != NULL);
    MIR_ASSERT(worker->current_task != NULL);
    if(!runtime->chunks_are_tasks)
    {
        // No task chunks. Associate current task with loop and return.
        worker->current_task->loop = loop;
        return;
    }

    struct mir_omp_team_t* team = worker->current_task->team;
    MIR_ASSERT(team != NULL);

    // Mark loop descriptor as stemming from non-parallel loop start routine.
    mir_lock_set(&(loop->lock));
    loop->non_parallel_start = 1;
    mir_lock_unset(&(loop->lock));

    // Create fake loop task on current worker.
    struct mir_task_t* task = mir_task_create_common(worker->current_task->func, worker->current_task->data, 0, 0, NULL, name, team, loop, worker->current_task);
    MIR_CHECK_MEM(task != NULL);

    // Write chunk details as metadata
    mir_task_write_metadata(task, "chunk_start");

    // Start profiling and book-keeping for fake task
    mir_task_execute_prolog(task);
}/*}}}*/

static void chunk_task_next(long start, long end, bool last)
{/*{{{*/
    if(!runtime->chunks_are_tasks)
        return;

    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(worker != NULL);
    MIR_ASSERT(worker->current_task != NULL);

    char str[MIR_SHORT_NAME_LEN];
    struct mir_task_t* temp = worker->current_task;

    // End current task.
    mir_task_execute_epilog(worker->current_task);

    if(last && temp->loop->non_parallel_start == 1)
    {
        // Mark that fake tasks are done.
        mir_task_wait();
    }
    else
    {
        // If loop stems from a parallel start routine, then the continuation is
        // part of the fake chunk tasks. Therefore we terminate current fake task
        // and create the continuation.
        //
        // If loop stems from a non-parallel start routine, then a continuation exists
        // independent of the fake chunk tasks. Therefore the current fake task can be
        // terminated without creating a continuation.

        if(last)
        {
            // Describe twin as continuation.
            strcpy(str, "chunk_continuation");
        }
        else
        {
            // Write chunk details as metadata.
            sprintf(str, "chunk_%lu_%lu", start, end);
        }

        // Create fake twin task.
        struct mir_task_t* twin = mir_task_create_twin(temp->name, temp, str);

        // Start profiling and book-keeping for fake twin task
        mir_task_execute_prolog(twin);
    }
}/*}}}*/

/* barrier.c */

void GOMP_barrier(void)
{ /*{{{*/
    struct mir_worker_t* worker = mir_worker_get_context();
    struct mir_omp_team_t* team;
    team = worker->current_task ? worker->current_task->team : NULL;
    if (!runtime->single_parallel_block && team) {
        // Announce impending barrier.
        __sync_fetch_and_add(&team->barrier_impending_count, 1);
        while(team->barrier_impending_count < team->num_threads)
        {
            // Execute tasks in system until all threads announce impending barrier.
            mir_worker_do_work(worker, MIR_WORKER_BACKOFF_DURING_BARRIER_WAIT);
        }

        // Wait for barrier.
        if(mir_barrier_wait(&team->barrier))
        {
            // Only one thread is allowed into this block.
            // Reset barrier impending count.
            team->barrier_impending_count = 0;
        }

    }
} /*}}}*/

/* critical.c */

void GOMP_critical_start(void)
{ /*{{{*/
    // A single global (whole program, not restricted to current team) critical section is supported.
    // From OpenMP 4.0 specification:
    // The binding thread set for a critical region is all threads in the contention group. Region execution is restricted to a single thread at a time among all threads in the contention group, without regard to the team(s) to which the threads belong.
    // The critical construct enforces exclusive access with respect to all critical constructs with the same name in all threads in the contention group, not just those threads in the current team.
    mir_lock_set(&runtime->omp_critsec_lock);
} /*}}}*/

void GOMP_critical_end(void)
{ /*{{{*/
    mir_lock_unset(&runtime->omp_critsec_lock);
} /*}}}*/

void GOMP_critical_name_start(void **pptr)
{ /*{{{*/
    if(*pptr == NULL)
    {
        // Create new critical section.
        struct mir_lock_t* lock = mir_malloc_int(sizeof(struct mir_lock_t));
        MIR_CHECK_MEM(lock != NULL);
        mir_lock_create(lock);

        // Pass lock information to caller.
        *pptr = lock;
    }

    mir_lock_set((struct mir_lock_t*)(*pptr));
} /*}}}*/

void GOMP_critical_name_end(void **pptr)
{ /*{{{*/
    MIR_ASSERT_STR(*pptr != NULL, "Named critical section lock is corrupted.");
    mir_lock_unset((struct mir_lock_t*)(*pptr));
} /*}}}*/

void GOMP_atomic_start(void)
{ /*{{{*/
    mir_lock_set(&runtime->omp_atomic_lock);
} /*}}}*/

void GOMP_atomic_end(void)
{ /*{{{*/
    mir_lock_unset(&runtime->omp_atomic_lock);
} /*}}}*/

/* loop.c, iter.c, env.c*/

static bool GOMP_loop_guided_next_int(long* pstart, long* pend)
{ /*{{{*/
    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(worker != NULL);
    MIR_ASSERT(worker->current_task != NULL);
    MIR_ASSERT(worker->current_task->loop != NULL);
    MIR_ASSERT(worker->current_task->loop->init == 1);

    struct mir_loop_des_t* loop = worker->current_task->loop;

    unsigned long nthreads = omp_get_num_threads();

    unsigned long n, q;
    long start, end;

    if (loop->next == loop->end)
        return false;

    start = loop->next;
    n = (loop->end - start) / loop->incr;
    q = (n + nthreads - 1) / nthreads;

    if (q < loop->chunk_size)
        q = loop->chunk_size;
    if (q <= n)
        end = start + q * loop->incr;
    else
        end = loop->end;

    loop->next = end;
    *pstart = start;
    *pend = end;
    return true;
} /*}}}*/

bool GOMP_loop_guided_next(long* istart, long* iend)
{ /*{{{*/
    bool ret;

    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(worker != NULL);
    MIR_ASSERT(worker->current_task != NULL);
    MIR_ASSERT(worker->current_task->loop != NULL);
    MIR_ASSERT(worker->current_task->loop->init == 1);

    struct mir_loop_des_t* loop = worker->current_task->loop;

    mir_lock_set(&(loop->lock));
    ret = GOMP_loop_guided_next_int(istart, iend);
    mir_lock_unset(&(loop->lock));
    chunk_task_next(*istart, *iend, !ret);

    return ret;
} /*}}}*/

bool GOMP_loop_guided_start (long start, long end, long incr, long chunk_size, long *istart, long *iend)
{ /*{{{*/
    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(worker != NULL);
    MIR_ASSERT(worker->current_task != NULL);
    MIR_ASSERT_STR(worker->current_task->loop == NULL, "Nested parallel for loops are not supported.");

    // Create common loop description within team.
    struct mir_omp_team_t* team = worker->current_task->team;
    MIR_ASSERT(team != NULL);
    mir_lock_set(&team->loop_lock);
    if(team->loop == NULL)
    {
        team->loop = mir_new_omp_loop_desc_init(start, end, incr, chunk_size, false);
    }
    mir_lock_unset(&team->loop_lock);
    chunk_task_start("GOMP_for_guided_task", team->loop);

    bool retval = GOMP_loop_guided_next(istart, iend);

    return retval;
} /*}}}*/

void GOMP_parallel_loop_guided_start (void (*fn) (void *), void *data, unsigned num_threads, long start, long end, long incr, long chunk_size)
{ /*{{{*/
    parallel_start(fn, data, num_threads, start, end, incr, chunk_size, true, false, "GOMP_parallel_for_guided_task");
} /*}}}*/

// Tasks spawned in GOMP_parallel_loop_guided have a single shared
// loop iteration allocator which assigns non-overlapping loop iterations
// on demand when GOMP_loop_guided_next_int is called. This is
// different from GOMP_parallel_loop_static.

void GOMP_parallel_loop_guided(void (*fn)(void*), void* data, unsigned num_threads, long start, long end, long incr, long chunk_size, unsigned flags)
{ /*{{{*/
    // Schedule for loop tasks on all workers except current.
    GOMP_parallel_loop_guided_start(fn, data, num_threads, start, end, incr, chunk_size);

    MIR_CONTEXT_EXIT;

    // Execute task.
    fn(data);

    MIR_CONTEXT_ENTER;

    // Wait for for loop tasks to finish.
    GOMP_parallel_end();
} /*}}}*/

static bool GOMP_loop_dynamic_next_int(long* pstart, long* pend)
{ /*{{{*/
    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(worker != NULL);
    MIR_ASSERT(worker->current_task != NULL);
    MIR_ASSERT(worker->current_task->loop != NULL);
    MIR_ASSERT(worker->current_task->loop->init == 1);

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
    MIR_ASSERT(worker->current_task->loop->init == 1);

    struct mir_loop_des_t* loop = worker->current_task->loop;

    mir_lock_set(&(loop->lock));
    ret = GOMP_loop_dynamic_next_int(istart, iend);
    mir_lock_unset(&(loop->lock));
    chunk_task_next(*istart, *iend, !ret);

    return ret;
} /*}}}*/

bool GOMP_loop_dynamic_start (long start, long end, long incr, long chunk_size, long *istart, long *iend)
{ /*{{{*/
    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(worker != NULL);
    MIR_ASSERT(worker->current_task != NULL);
    MIR_ASSERT_STR(worker->current_task->loop == NULL, "Nested parallel for loops are not supported.");

    // Create common loop description within team.
    struct mir_omp_team_t* team = worker->current_task->team;
    MIR_ASSERT(team != NULL);
    mir_lock_set(&team->loop_lock);
    if(team->loop == NULL)
    {
        team->loop = mir_new_omp_loop_desc_init(start, end, incr, chunk_size * incr, false);
    }
    mir_lock_unset(&team->loop_lock);
    chunk_task_start("GOMP_for_dynamic_task", team->loop);

    bool retval = GOMP_loop_dynamic_next(istart, iend);

    return retval;
} /*}}}*/

void GOMP_parallel_loop_dynamic_start (void (*fn) (void *), void *data, unsigned num_threads, long start, long end, long incr, long chunk_size)
{ /*{{{*/
    parallel_start(fn, data, num_threads, start, end, incr, chunk_size * incr, true, false, "GOMP_parallel_for_dynamic_task");
} /*}}}*/

// Tasks spawned in GOMP_parallel_loop_dynamic have a single shared
// loop iteration allocator which assigns non-overlapping loop iterations
// on demand when GOMP_loop_dynamic_next_int is called. This is
// different from GOMP_parallel_loop_static.

void GOMP_parallel_loop_dynamic(void (*fn)(void*), void* data, unsigned num_threads, long start, long end, long incr, long chunk_size, unsigned flags)
{ /*{{{*/
    // Schedule for loop tasks on all workers except current.
    GOMP_parallel_loop_dynamic_start(fn, data, num_threads, start, end, incr, chunk_size);

    MIR_CONTEXT_EXIT;

    // Execute task.
    fn(data);

    MIR_CONTEXT_ENTER;

    // Wait for for loop tasks to finish.
    GOMP_parallel_end();
} /*}}}*/

static int GOMP_loop_static_next_int(long* pstart, long* pend)
{ /*{{{*/
    unsigned long nthreads = omp_get_num_threads();

    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(worker != NULL);
    MIR_ASSERT(worker->current_task != NULL);
    MIR_ASSERT(worker->current_task->loop != NULL);
    MIR_ASSERT(worker->current_task->loop->init == 1);

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
    bool ret = !GOMP_loop_static_next_int(istart, iend);
    chunk_task_next(*istart, *iend, !ret);

    return ret;
} /*}}}*/

bool GOMP_loop_static_start (long start, long end, long incr, long chunk_size, long *istart, long *iend)
{ /*{{{*/
    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(worker != NULL);
    MIR_ASSERT(worker->current_task != NULL);
    MIR_ASSERT_STR(worker->current_task->loop == NULL, "Nested parallel for loops are not supported.");

    // Create loop description and associate with task.
    struct mir_loop_des_t* loop;
    loop = mir_new_omp_loop_desc_init(start, end, incr, chunk_size, false);
    chunk_task_start("GOMP_for_static_task", loop);

    bool retval = GOMP_loop_static_next(istart, iend);

    return retval;
} /*}}}*/

void GOMP_parallel_loop_static_start (void (*fn) (void *), void *data, unsigned num_threads, long start, long end, long incr, long chunk_size)
{ /*{{{*/
    parallel_start(fn, data, num_threads, start, end, incr, chunk_size, true, true, "GOMP_parallel_for_static_task");
} /*}}}*/

// Tasks spawned in GOMP_parallel_loop_static have their own local
// loop iteration allocators which assign pre-decided, non-overlapping
// loop iterations when GOMP_loop_static_next_int is called. This is
// different from GOMP_parallel_loop_dynamic.

void GOMP_parallel_loop_static(void (*fn)(void*), void* data, unsigned num_threads, long start, long end, long incr, long chunk_size, unsigned flags)
{ /*{{{*/
    // Schedule for loop tasks on all workers except current.
    GOMP_parallel_loop_static_start(fn, data, num_threads, start, end, incr, chunk_size);

    MIR_CONTEXT_EXIT;

    // Execute task.
    fn(data);

    MIR_CONTEXT_ENTER;

    // Wait for for loop tasks to finish.
    GOMP_parallel_end();
} /*}}}*/

bool GOMP_loop_auto_next(long* istart, long* iend)
{ /*{{{*/
    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(worker != NULL);
    MIR_ASSERT(worker->current_task != NULL);
    MIR_ASSERT(worker->current_task->loop != NULL);
    MIR_ASSERT(worker->current_task->loop->init == 1);

    bool ret = false;

    struct mir_loop_des_t* loop = worker->current_task->loop;
    if (loop->precomp_schedule_exists) {
        if (loop->precomp_schedule) {
            *istart = loop->precomp_schedule->chunk_start;
            *iend = loop->precomp_schedule->chunk_end;
            loop->precomp_schedule = loop->precomp_schedule->next;
            ret = true;
        }
    } else {
        // Revert to static schedule if precomputed schedule is absent.
        ret = !GOMP_loop_static_next_int(istart, iend);
    }

    chunk_task_next(*istart, *iend, !ret);

    return ret;
} /*}}}*/

bool GOMP_loop_auto_start (long start, long end, long incr, long chunk_size, long *istart, long *iend)
{ /*{{{*/
    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(worker != NULL);
    MIR_ASSERT(worker->current_task != NULL);
    MIR_ASSERT_STR(worker->current_task->loop == NULL, "Nested parallel for loops are not supported.");

    // Create loop description and associate with task.
    struct mir_loop_des_t* loop;
    loop = mir_new_omp_loop_desc_init(start, end, incr, chunk_size, true);
    chunk_task_start("GOMP_for_auto_task", loop);

    return GOMP_loop_auto_next(istart, iend);
} /*}}}*/

void GOMP_parallel_loop_auto_start (void (*fn) (void *), void *data, unsigned num_threads, long start, long end, long incr, long chunk_size)
{ /*{{{*/
    parallel_start(fn, data, num_threads, start, end, incr, chunk_size, true, true, "GOMP_parallel_for_auto_task");
} /*}}}*/

// Tasks spawned in GOMP_parallel_loop_auto have their own local
// loop iteration allocators which assign pre-decided, non-overlapping
// loop iterations when GOMP_loop_auto_next_int is called. This is
// different from GOMP_parallel_loop_dynamic.

void GOMP_parallel_loop_auto(void (*fn)(void*), void* data, unsigned num_threads, long start, long end, long incr, long chunk_size, unsigned flags)
{ /*{{{*/
    // Schedule for loop tasks on all workers except current.
    GOMP_parallel_loop_auto_start(fn, data, num_threads, start, end, incr, chunk_size);

    MIR_CONTEXT_EXIT;

    // Execute task.
    fn(data);

    MIR_CONTEXT_ENTER;

    // Wait for for loop tasks to finish.
    GOMP_parallel_end();
} /*}}}*/

static int parse_omp_schedule_chunk_size(void)
{ /*{{{*/
    char* env, *end;
    unsigned long value = 1;
    enum omp_for_schedule_t omp_for_schedule = OFS_DYNAMIC;

    env = getenv("OMP_SCHEDULE");
    if (env == NULL)
        return value;

    while (isspace(*env))
        ++env;
    if (strncasecmp(env, "static", 6) == 0) {
        omp_for_schedule = OFS_STATIC;
        env += 6;
    }
    else if (strncasecmp(env, "dynamic", 7) == 0) {
        omp_for_schedule = OFS_DYNAMIC;
        env += 7;
    }
    else if (strncasecmp(env, "guided", 6) == 0) {
        omp_for_schedule = OFS_GUIDED;
        env += 6;
    }
    else if (strncasecmp(env, "auto", 4) == 0) {
        omp_for_schedule = OFS_AUTO;
        env += 4;
    }
    else
        goto unknown;

    while (isspace(*env))
        ++env;
    if (*env == '\0') {
        value = omp_for_schedule != OFS_STATIC;
        return value;
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

    if (value == 0 && omp_for_schedule != OFS_STATIC)
        value = 1;
    return value;

unknown:
    MIR_LOG_ERR("Unknown value for OMP_SCHEDULE.");

invalid:
    MIR_LOG_ERR("Invalid value for chunk size in OMP_SCHEDULE.");
} /*}}}*/

static enum omp_for_schedule_t parse_omp_schedule_name(void)
{ /*{{{*/
    char* env, *end;
    enum omp_for_schedule_t omp_for_schedule = OFS_DYNAMIC;

    env = getenv("OMP_SCHEDULE");
    if (env == NULL)
        return omp_for_schedule;

    while (isspace(*env))
        ++env;
    if (strncasecmp(env, "static", 6) == 0) {
        omp_for_schedule = OFS_STATIC;
        env += 6;
    }
    else if (strncasecmp(env, "dynamic", 7) == 0) {
        omp_for_schedule = OFS_DYNAMIC;
        env += 7;
    }
    else if (strncasecmp(env, "guided", 6) == 0) {
        omp_for_schedule = OFS_GUIDED;
        env += 6;
    }
    else if (strncasecmp(env, "auto", 4) == 0) {
        omp_for_schedule = OFS_AUTO;
        env += 4;
    }
    else
        goto unknown;

    return omp_for_schedule;

unknown:
    MIR_LOG_ERR("Unknown value for OMP_SCHEDULE.");
} /*}}}*/

void parse_omp_schedule(void)
{ /*{{{*/
    runtime->omp_for_schedule = parse_omp_schedule_name();
    runtime->omp_for_chunk_size = parse_omp_schedule_chunk_size();
} /*}}}*/

bool GOMP_loop_runtime_next(long* istart, long* iend)
{ /*{{{*/
    bool retval = false;

    switch (runtime->omp_for_schedule) {
    case OFS_STATIC:
        retval = GOMP_loop_static_next(istart, iend);
        break;
    case OFS_DYNAMIC:
        retval = GOMP_loop_dynamic_next(istart, iend);
        break;
    case OFS_GUIDED:
        retval = GOMP_loop_guided_next(istart, iend);
        break;
    case OFS_AUTO:
        // The auto schedule uses a precomputed schedule if available or reverts to static schedule.
        retval = GOMP_loop_auto_next(istart, iend);
        break;
    default:
        MIR_LOG_ERR("OMP_SCHEDULE is unsupported.");
        break;
    }

    return retval;
} /*}}}*/

bool GOMP_loop_runtime_start (long start, long end, long incr, long *istart, long *iend)
{ /*{{{*/
    bool retval = false;

    switch (parse_omp_schedule_name()) {
    case OFS_STATIC:
        retval = GOMP_loop_static_start(start, end, incr, parse_omp_schedule_chunk_size(), istart, iend);
        break;
    case OFS_DYNAMIC:
        retval = GOMP_loop_dynamic_start(start, end, incr, parse_omp_schedule_chunk_size(), istart, iend);
        break;
    case OFS_GUIDED:
        retval = GOMP_loop_guided_start(start, end, incr, parse_omp_schedule_chunk_size(), istart, iend);
        break;
    case OFS_AUTO:
        // The auto schedule uses a precomputed schedule if available or reverts to static schedule.
        retval = GOMP_loop_auto_start(start, end, incr, parse_omp_schedule_chunk_size(), istart, iend);
        break;
    default:
        MIR_LOG_ERR("OMP_SCHEDULE is unsupported.");
        break;
    }

    return retval;
} /*}}}*/

void GOMP_parallel_loop_runtime_start(void (*fn) (void *), void *data,
                                      unsigned num_threads, long start,
                                      long end, long incr)
{ /*{{{*/
    switch (parse_omp_schedule_name()) {
    case OFS_STATIC:
        GOMP_parallel_loop_static_start(fn, data, num_threads, start, end, incr, parse_omp_schedule_chunk_size());
        break;
    case OFS_DYNAMIC:
        GOMP_parallel_loop_dynamic_start(fn, data, num_threads, start, end, incr, parse_omp_schedule_chunk_size());
        break;
    case OFS_GUIDED:
        GOMP_parallel_loop_guided_start(fn, data, num_threads, start, end, incr, parse_omp_schedule_chunk_size());
        break;
    case OFS_AUTO:
        GOMP_parallel_loop_auto_start(fn, data, num_threads, start, end, incr, parse_omp_schedule_chunk_size());
        break;
    default:
        MIR_LOG_ERR("OMP_SCHEDULE is unsupported.");
    }
} /*}}}*/

void GOMP_parallel_loop_runtime(void (*fn)(void*), void* data, unsigned num_threads, long start, long end, long incr, unsigned flags)
{ /*{{{*/
    // Schedule for loop tasks on all workers except current.
    GOMP_parallel_loop_runtime_start(fn, data, num_threads, start, end, incr);

    MIR_CONTEXT_EXIT;

    // Execute task.
    fn(data);

    MIR_CONTEXT_ENTER;

    // Wait for for loop tasks to finish.
    GOMP_parallel_end();
} /*}}}*/

void GOMP_loop_end_int()
{/*{{{*/
    // Wait for all workers to finish executing their loop task.
    // Waiting is essential since loop description are deleted next.
    GOMP_barrier();

    // Delete loop description associated with task.
    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(worker != NULL);
    MIR_ASSERT(worker->current_task != NULL);
    worker->current_task->loop = NULL;

    // Delete loop description associated with team.
    struct mir_omp_team_t* team = worker->current_task->team;
    MIR_ASSERT(team != NULL);
    mir_lock_set(&team->loop_lock);
    team->loop = NULL;
    mir_lock_unset(&team->loop_lock);
}/*}}}*/

void GOMP_loop_end(void)
{ /*{{{*/
    GOMP_loop_end_int();
} /*}}}*/

void GOMP_loop_end_nowait(void)
{ /*{{{*/
    GOMP_loop_end_int();
} /*}}}*/

/* parallel.c */

void GOMP_parallel_start(void (*fn)(void*), void* data, unsigned num_threads)
{ /*{{{*/
    parallel_start(fn, data, num_threads, 0, 0, 0, 0, false, false, "GOMP_parallel_task");
} /*}}}*/

void GOMP_parallel_end(void)
{ /*{{{*/
    struct mir_worker_t* worker = mir_worker_get_context();
    struct mir_omp_team_t* team;
    team = worker->current_task ? worker->current_task->team : NULL;

    // Stop profiling and book-keeping for fake task.
    MIR_ASSERT(worker->current_task != NULL);
    mir_task_execute_epilog(worker->current_task);

    // Ensure tasks are done.
    mir_task_wait();

    // Last task so unlink team.
    if (team != NULL)
        team->prev = NULL;

    mir_soft_destroy();
} /*}}}*/

void GOMP_parallel(void (*fn)(void*), void* data, unsigned num_threads, unsigned flags)
{ /*{{{*/
    // Schedule parallel block tasks on all workers except current.
    GOMP_parallel_start(fn, data, num_threads);

    MIR_CONTEXT_EXIT;

    // Execute parallel block task.
    fn(data);

    MIR_CONTEXT_ENTER;

    // Wait for parallel block tasks to finish.
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

    if (team && runtime->num_workers != team->num_threads)
        MIR_LOG_ERR("Combining tasks and parallel sections specifying num_threads is not supported.");

    char* buf = NULL;
    if (copyfn) {
        buf = mir_malloc_int(sizeof(char) * arg_size);
        MIR_CHECK_MEM(buf != NULL);
        copyfn(buf, data);
    }

    mir_task_create_on_worker((mir_tfunc_t)fn, buf ? buf : data, (size_t)(arg_size), 0, NULL, task_name, team, NULL, -1);
} /*}}}*/

__attribute__((optimize("-fno-optimize-sibling-calls")))
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

    if (team == NULL) {
        return true;
    }

    int sc = __sync_fetch_and_sub(&team->single_count, 1);
    if (sc == 1) {
        __sync_bool_compare_and_swap(&team->single_count, 0,
            team->num_threads);
    }
    return runtime->single_parallel_block || sc == team->num_threads;
} /*}}}*/

/* omp.h */

int omp_get_thread_num(void)
{ /*{{{*/
    if (runtime == NULL) {
        return 0;
    }

    struct mir_worker_t* worker = mir_worker_get_context();

    return worker->id;
} /*}}}*/

int omp_get_num_threads(void)
{ /*{{{*/
    if(runtime == NULL) {
        return 1;
    }

    struct mir_worker_t* worker = mir_worker_get_context();
    struct mir_omp_team_t* team = worker->current_task ? worker->current_task->team : NULL;

    return team ? team->num_threads : 1;
} /*}}}*/

int omp_get_max_threads(void)
{ /*{{{*/
    int retval = (runtime == NULL ?  mir_arch_create_by_query()->num_cores : runtime->num_workers);

    return retval;
} /*}}}*/

void omp_set_num_threads (int n)
{/*{{{*/
    MIR_LOG_ERR("omp_set_num_threads() is not supported.");
}/*}}}*/

void omp_set_dynamic(int val)
{/*{{{*/
    MIR_LOG_ERR("omp_set_dynamic() is not supported.");
}/*}}}*/

int omp_get_dynamic(void)
{/*{{{*/
    MIR_LOG_ERR("omp_get_dynamic() is not supported.");
}/*}}}*/

void omp_set_nested(int val)
{/*{{{*/
    MIR_LOG_ERR("omp_set_nested() is not supported.");
}/*}}}*/

int omp_get_nested (void)
{/*{{{*/
    MIR_LOG_ERR("omp_get_nested() is not supported.");
}/*}}}*/

void omp_set_schedule (omp_sched_t kind, int modifier)
{/*{{{*/
    MIR_LOG_ERR("omp_set_schedule() is not supported.");
}/*}}}*/

void omp_get_schedule (omp_sched_t *kind, int *modifier)
{/*{{{*/
    MIR_LOG_ERR("omp_get_schedule() is not supported.");
}/*}}}*/

int omp_get_thread_limit (void)
{/*{{{*/
    MIR_LOG_ERR("omp_get_thread_limit() is not supported.");
}/*}}}*/

void omp_set_max_active_levels (int max_levels)
{/*{{{*/
    MIR_LOG_ERR("omp_set_max_active_levels() is not supported.");
}/*}}}*/

int omp_get_max_active_levels (void)
{/*{{{*/
    MIR_LOG_ERR("omp_get_max_active_levels() is not supported.");
}/*}}}*/

int omp_get_cancellation (void)
{/*{{{*/
    MIR_LOG_ERR("omp_get_cancellation() is not supported.");
}/*}}}*/

omp_proc_bind_t omp_get_proc_bind (void)
{/*{{{*/
    MIR_LOG_ERR("omp_get_proc_bind() is not supported.");
}/*}}}*/

void omp_set_default_device (int device_num)
{/*{{{*/
    MIR_LOG_ERR("omp_set_default_device() is not supported.");
}/*}}}*/

int omp_get_default_device (void)
{/*{{{*/
    MIR_LOG_ERR("omp_get_default_device() is not supported.");
}/*}}}*/

int omp_get_num_devices (void)
{/*{{{*/
    MIR_LOG_ERR("omp_get_num_devices() is not supported.");
}/*}}}*/

int omp_get_num_teams (void)
{/*{{{*/
    MIR_LOG_ERR("omp_get_num_teams() is not supported.");
}/*}}}*/

int omp_get_team_num (void)
{/*{{{*/
    MIR_LOG_ERR("omp_get_team_num() is not supported.");
}/*}}}*/

int omp_is_initial_device (void)
{/*{{{*/
    MIR_LOG_ERR("omp_is_initial_device() is not supported.");
}/*}}}*/

int omp_get_num_procs (void)
{/*{{{*/
    MIR_LOG_ERR("omp_get_num_procs() is not supported.");
}/*}}}*/

int omp_in_parallel (void)
{/*{{{*/
    MIR_LOG_ERR("omp_in_parallel() is not supported.");
}/*}}}*/

void omp_init_lock (omp_lock_t * lock)
{/*{{{*/
    MIR_LOG_ERR("omp_init_lock() is not supported.");
}/*}}}*/

void omp_destroy_lock (omp_lock_t * lock)
{/*{{{*/
    MIR_LOG_ERR("omp_destroy_lock() is not supported.");
}/*}}}*/

void omp_set_lock (omp_lock_t * lock)
{/*{{{*/
    MIR_LOG_ERR("omp_set_lock() is not supported.");
}/*}}}*/

void omp_unset_lock (omp_lock_t * lock)
{/*{{{*/
    MIR_LOG_ERR("omp_unset_lock() is not supported.");
}/*}}}*/

int omp_test_lock (omp_lock_t * lock)
{/*{{{*/
    MIR_LOG_ERR("omp_test_lock() is not supported.");
}/*}}}*/

void omp_init_nest_lock (omp_nest_lock_t * lock)
{/*{{{*/
    MIR_LOG_ERR("omp_init_nest_lock() is not supported.");
}/*}}}*/

void omp_destroy_nest_lock (omp_nest_lock_t * lock)
{/*{{{*/
    MIR_LOG_ERR("omp_destroy_nest_lock() is not supported.");
}/*}}}*/

void omp_set_nest_lock (omp_nest_lock_t * lock)
{/*{{{*/
    MIR_LOG_ERR("omp_set_nest_lock() is not supported.");
}/*}}}*/

void omp_unset_nest_lock (omp_nest_lock_t * lock)
{/*{{{*/
    MIR_LOG_ERR("omp_unset_nest_lock() is not supported.");
}/*}}}*/

int omp_test_nest_lock (omp_nest_lock_t * lock)
{/*{{{*/
    MIR_LOG_ERR("omp_test_nest_lock() is not supported.");
}/*}}}*/

double omp_get_wtime (void)
{/*{{{*/
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0)
        MIR_LOG_ERR("gettimeofday() failed.");

    return tv.tv_sec + (double) tv.tv_usec / 1000000.0;
}/*}}}*/

double omp_get_wtick (void)
{/*{{{*/
    MIR_LOG_ERR("omp_get_wtick() is not supported.");
}/*}}}*/

int omp_get_level (void)
{/*{{{*/
    MIR_LOG_ERR("omp_get_level() is not supported.");
}/*}}}*/

int omp_get_ancestor_thread_num (int level)
{/*{{{*/
    MIR_LOG_ERR("omp_get_ancestor_thread_num() is not supported.");
}/*}}}*/

int omp_get_team_size (int level)
{/*{{{*/
    MIR_LOG_ERR("omp_get_team_size() is not supported.");
}/*}}}*/

int omp_get_active_level (void)
{/*{{{*/
    MIR_LOG_ERR("omp_get_active_level() is not supported.");
}/*}}}*/

int omp_in_final (void)
{/*{{{*/
    MIR_LOG_ERR("omp_in_final() is not supported.");
}/*}}}*/
