#include "mir_omp_int.h"
#include "mir_task.h"
#include "mir_debug.h"
#include "mir_runtime.h"
#include "mir_worker.h"

extern struct mir_runtime_t* runtime;

/* barrier.c */

void GOMP_barrier (void)
{
    // Implicit tasks fuck with profiling.
    /*mir_twc_wait_pw();*/
    MIR_ABORT(MIR_ERROR_STR "GOMP_barrier not implemented yet!\n");
}
bool GOMP_barrier_cancel (void)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_barrier_cancel not implemented yet!\n");
}

/* critical.c */

void GOMP_critical_start (void)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_critical_start not implemented yet!\n");
}
void GOMP_critical_end (void)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_critical_end not implemented yet!\n");
}
void GOMP_critical_name_start (void ** a0)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_critical_name_start not implemented yet!\n");
}
void GOMP_critical_name_end (void ** a1)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_critical_name_end not implemented yet!\n");
}
void GOMP_atomic_start (void)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_atomic_start not implemented yet!\n");
}
void GOMP_atomic_end (void)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_atomic_end not implemented yet!\n");
}

/* loop.c */


bool GOMP_loop_static_start (long a0, long a1, long a2, long a3, long * a4, long * a5)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_static_start not implemented yet!\n");
}
bool GOMP_loop_dynamic_start (long a0, long a1, long a2, long a3, long * a4, long * a5)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_dynamic_start not implemented yet!\n");
}
bool GOMP_loop_guided_start (long a0, long a1, long a2, long a3, long * a4, long * a5)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_guided_start not implemented yet!\n");
}
bool GOMP_loop_runtime_start (long a0, long a1, long a2, long *a3, long * a4)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_runtime_start not implemented yet!\n");
}

bool GOMP_loop_ordered_static_start (long a0, long a1, long a2, long a3, long * a4, long * a5) 
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_ordered_static_start not implemented yet!\n");
}
bool GOMP_loop_ordered_dynamic_start (long a0, long a1, long a2, long a3, long * a4, long * a5) 
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_ordered_dynamic_start not implemented yet!\n");
}
bool GOMP_loop_ordered_guided_start (long a0, long a1, long a2, long a3, long * a4, long * a5) 
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_ordered_guided_start not implemented yet!\n");
}
bool GOMP_loop_ordered_runtime_start (long a0, long a1, long a2, long *a3, long * a4) 
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_ordered_runtime_start not implemented yet!\n");
}

bool GOMP_loop_static_next (long * a0, long * a1)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_static_next not implemented yet!\n");
}
bool GOMP_loop_dynamic_next(long * a0, long * a1) 
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_dynamic_next not implemented yet!\n");
}
bool GOMP_loop_guided_next(long * a0, long * a1) 
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_guided_next not implemented yet!\n");
}
bool GOMP_loop_runtime_next(long * a0, long * a1) 
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_runtime_next not implemented yet!\n");
}

bool GOMP_loop_ordered_static_next(long * a0, long * a1) 
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_ordered_static_next not implemented yet!\n");
}
bool GOMP_loop_ordered_dynamic_next(long * a0, long * a1) 
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_ordered_dynamic_next not implemented yet!\n");
}
bool GOMP_loop_ordered_guided_next(long * a0, long * a1) 
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_ordered_guided_next not implemented yet!\n");
}
bool GOMP_loop_ordered_runtime_next(long * a0, long * a1) 
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_ordered_runtime_next not implemented yet!\n");
}

void GOMP_parallel_loop_static_start (void (*a0)(void *), void * a1, unsigned a2, long a3, long a4, long a5, long a6)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_parallel_loop_static_start not implemented yet!\n");
}
void GOMP_parallel_loop_dynamic_start(void (*a0)(void *), void * a1, unsigned a2, long a3, long a4, long a5, long a6) 
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_parallel_loop_dynamic_start not implemented yet!\n");
}
void GOMP_parallel_loop_guided_start(void (*a0)(void *), void * a1, unsigned a2, long a3, long a4, long a5, long a6) 
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_parallel_loop_guided_start not implemented yet!\n");
}
void GOMP_parallel_loop_runtime_start(void (*a0)(void *), void * a1, unsigned a2, long a3, long a4, long a5) 
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_parallel_loop_runtime_start not implemented yet!\n");
}
void GOMP_parallel_loop_static(void (*a0)(void *), void * a1, unsigned a2, long a3, long a4, long a5, long a6, unsigned a7) 
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_parallel_loop_static not implemented yet!\n");
}
void GOMP_parallel_loop_dynamic(void (*a0)(void *), void * a1, unsigned a2, long a3, long a4, long a5, long a6, unsigned a7) 
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_parallel_loop_dynamic not implemented yet!\n");
}
void GOMP_parallel_loop_guided (void (*a0)(void *), void *a1, unsigned a2, long a3, long a4, long a5, long a6, unsigned a7)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_parallel_loop_guided not implemented yet!\n");
}
void GOMP_parallel_loop_runtime (void (*a0)(void *), void * a1, unsigned a2, long a3, long a4, long a5, unsigned a6)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_parallel_loop_runtime not implemented yet!\n");
}

void GOMP_loop_end (void)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_end not implemented yet!\n");
}
void GOMP_loop_end_nowait (void)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_end_nowait not implemented yet!\n");
}
bool GOMP_loop_end_cancel (void)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_end_cancel not implemented yet!\n");
}

/* loop_ull.c */

bool GOMP_loop_ull_static_start (bool a0, unsigned long long a1, unsigned long long a2, unsigned long long a3, unsigned long long a4, unsigned long long * a5, unsigned long long * a6)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_ull_static_start not implemented yet!\n");
}
bool GOMP_loop_ull_dynamic_start (bool a0, unsigned long long a1, unsigned long long a2, unsigned long long a3, unsigned long long a4, unsigned long long * a5, unsigned long long * a6)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_ull_dynamic_start not implemented yet!\n");
}
bool GOMP_loop_ull_guided_start (bool a0, unsigned long long a1, unsigned long long a2, unsigned long long a3, unsigned long long a4, unsigned long long * a5, unsigned long long * a6)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_ull_guided_start not implemented yet!\n");
}
bool GOMP_loop_ull_runtime_start (bool a0, unsigned long long a1, unsigned long long a2, unsigned long long a3, unsigned long long * a4, unsigned long long * a5)
{
    MIR_ABORT(MIR_ERROR_STR  "GOMP_loop_ull_runtime_start not implemented yet!\n");
}
bool GOMP_loop_ull_ordered_static_start (bool a0, unsigned long long a1, unsigned long long a2, unsigned long long a3, unsigned long long a4, unsigned long long * a5, unsigned long long * a6)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_ull_ordered_static_start not implemented yet!\n");
}
bool GOMP_loop_ull_ordered_dynamic_start (bool a0, unsigned long long a1, unsigned long long a2, unsigned long long a3, unsigned long long a4, unsigned long long * a5, unsigned long long * a6)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_ull_ordered_dynamic_start not implemented yet!\n");
}
bool GOMP_loop_ull_ordered_guided_start (bool a0, unsigned long long a1, unsigned long long a2, unsigned long long a3, unsigned long long a4, unsigned long long * a5, unsigned long long * a6)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_ull_ordered_guided_start not implemented yet!\n");
}
bool GOMP_loop_ull_ordered_runtime_start (bool a0, unsigned long long a1, unsigned long long a2, unsigned long long a3, unsigned long long * a4, unsigned long long * a5)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_ull_ordered_runtime_start not implemented yet!\n");
}

bool GOMP_loop_ull_static_next (unsigned long long * a0, unsigned long long * a1)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_ull_static_next not implemented yet!\n");
}
bool GOMP_loop_ull_dynamic_next (unsigned long long * a0, unsigned long long * a1)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_ull_dynamic_next not implemented yet!\n");
}
bool GOMP_loop_ull_guided_next (unsigned long long * a0, unsigned long long * a1)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_ull_guided_next not implemented yet!\n");
}
bool GOMP_loop_ull_runtime_next (unsigned long long * a0, unsigned long long * a1)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_ull_runtime_next not implemented yet!\n");
}

bool GOMP_loop_ull_ordered_static_next (unsigned long long * a0, unsigned long long * a1)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_ull_ordered_static_next not implemented yet!\n");
}
bool GOMP_loop_ull_ordered_dynamic_next (unsigned long long * a0, unsigned long long * a1)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_ull_ordered_dynamic_next not implemented yet!\n");
}
bool GOMP_loop_ull_ordered_guided_next (unsigned long long * a0, unsigned long long * a1)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_ull_ordered_guided_next not implemented yet!\n");
}
bool GOMP_loop_ull_ordered_runtime_next (unsigned long long * a0, unsigned long long * a1)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_loop_ull_ordered_runtime_next not implemented yet!\n");
}

/* ordered.c */

void GOMP_ordered_start (void)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_ordered_start not implemented yet!\n");
}
void GOMP_ordered_end (void)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_ordered_end not implemented yet!\n");
}

/* parallel.c */

void GOMP_parallel_start (void (*fn) (void *), void * data, unsigned num_threads)
{
    // Implicit tasks fuck with profiling.
    /*MIR_DEBUG(MIR_DEBUG_STR "Note: GOMP_parallel_start implementation ignores num_threads argument!\n");*/
    /*for(int i=1; i<runtime->num_workers; i++)*/
    /*{*/
        /*mir_task_create_on_pw((mir_tfunc_t) fn, (void*) data, (size_t)(0), 0, NULL, NULL, i);*/
    /*}*/
    MIR_ABORT(MIR_ERROR_STR "GOMP_parallel_start not implemented yet!\n");
}
void GOMP_parallel_end (void)
{
    // Implicit tasks fuck with profiling.
    /*mir_twc_wait_pw();*/
    MIR_ABORT(MIR_ERROR_STR "GOMP_parallel_end not implemented yet!\n");
}
void GOMP_parallel (void (*fn) (void *), void *data, unsigned num_threads, unsigned flags)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_parallel not implemented yet!\n");
}
bool GOMP_cancel (int a0, bool a1)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_cancel not implemented yet!\n");
}
bool GOMP_cancellation_point (int a0)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_cancellation_point not implemented yet!\n");
}

/* task.c */

void GOMP_task(void (*fn)(void *), void *data, void (*copyfn)(void *, void *), long arg_size, long arg_align, bool if_clause, unsigned flags, void** deps)
{
    struct mir_task_t* task = mir_task_create_pw((mir_tfunc_t) fn, (void*) data, (size_t)(arg_size), 0, NULL, NULL);
    return;
}
void GOMP_taskwait (void)
{
    mir_twc_wait_pw();
}
void GOMP_taskyield (void)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_taskyield not implemented yet!\n");
}
void GOMP_taskgroup_start (void)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_taskgroup_start not implemented yet!\n");
}
void GOMP_taskgroup_end (void)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_taskgroup_end not implemented yet!\n");
}

/* sections.c */

unsigned GOMP_sections_start (unsigned a0)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_sections_start not implemented yet!\n");
}
unsigned GOMP_sections_next (void)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_sections_next not implemented yet!\n");
}
void GOMP_parallel_sections_start (void (*a0) (void *), void * a1, unsigned a2, unsigned a3)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_parallel_sections_start not implemented yet!\n");
}
void GOMP_parallel_sections (void (*a0) (void *), void * a1, unsigned a2, unsigned a3, unsigned a4)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_parallel_sections not implemented yet!\n");
}
void GOMP_sections_end (void)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_sections_end not implemented yet!\n");
}
void GOMP_sections_end_nowait (void)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_sections_end_nowait not implemented yet!\n");
}
bool GOMP_sections_end_cancel (void)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_sections_end_cancel not implemented yet!\n");
}

/* single.c */

bool GOMP_single_start (void)
{
    // Implicit tasks fuck with profiling.
    /*// FIXME: Naive implementation*/
    /*struct mir_worker_t* worker = mir_worker_get_context(); */
    /*if(worker->id == 0)*/
        /*return true;*/
    /*else*/
        /*return false;*/
    MIR_ABORT(MIR_ERROR_STR "*GOMP_single_start not implemented yet!\n");
}
void *GOMP_single_copy_start (void)
{
    MIR_ABORT(MIR_ERROR_STR "*GOMP_single_copy_start not implemented yet!\n");
}
void GOMP_single_copy_end (void * a0)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_single_copy_end not implemented yet!\n");
}

/* target.c */

void GOMP_target (int a0, void (*a1)(void *), const void * a2, size_t a3, void ** a4, size_t * a5, unsigned char * a6)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_target not implemented yet!\n");
}
void GOMP_target_data (int a0, const void * a1, size_t a2, void ** a3, size_t * a4, unsigned char * a5)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_target_data not implemented yet!\n");
}
void GOMP_target_end_data (void)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_target_end_data not implemented yet!\n");
}
void GOMP_target_update (int a0, const void * a1, size_t a2, void ** a3, size_t * a4, unsigned char * a5)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_target_update not implemented yet!\n");
}
void GOMP_teams (unsigned int a0, unsigned int a1)
{
    MIR_ABORT(MIR_ERROR_STR "GOMP_teams not implemented yet!\n");
}
