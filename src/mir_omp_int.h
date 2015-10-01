#ifndef MIR_OMP_INT_H
#define MIR_OMP_INT_H

#include <stdbool.h>

#include "mir_types.h"

BEGIN_C_DECLS

void omp_init(void);
void omp_destroy(void);

enum omp_for_schedule_t {
    OFS_RUNTIME,
    OFS_STATIC,
    OFS_DYNAMIC,
    OFS_GUIDED,
    OFS_AUTO
};

/* Refactored from GCC git repository git://gcc.gnu.org/git/gcc.git HEAD ae76874abdf11bb77597f7285cb115bd78e82fda */
// FIXME: Warning!! Fragile!! Potential version mismatch when linked with different libgomp on system. Add a check!

/* env.c */
void parse_omp_schedule(void);

/* barrier.c */

void GOMP_barrier(void);

/* critical.c */

void GOMP_critical_start(void);
void GOMP_critical_end(void);
void GOMP_atomic_start(void);
void GOMP_atomic_end(void);

/* loop.c */

bool GOMP_loop_guided_next(long* istart, long* iend);
bool GOMP_loop_guided_start (long start, long end, long incr, long chunk_size, long *istart, long *iend);
void GOMP_parallel_loop_guided_start (void (*fn) (void *), void *data, unsigned num_threads, long start, long end, long incr, long chunk_size);
void GOMP_parallel_loop_guided(void (*fn)(void*), void* data, unsigned num_threads, long start, long end, long incr, long chunk_size, unsigned flags);
bool GOMP_loop_dynamic_next(long* istart, long* iend);
bool GOMP_loop_dynamic_start (long start, long end, long incr, long chunk_size, long *istart, long *iend);
void GOMP_parallel_loop_dynamic_start (void (*fn) (void *), void *data, unsigned num_threads, long start, long end, long incr, long chunk_size);
void GOMP_parallel_loop_dynamic(void (*fn)(void*), void* data, unsigned num_threads, long start, long end, long incr, long chunk_size, unsigned flags);
bool GOMP_loop_static_next(long* istart, long* iend);
bool GOMP_loop_static_start (long start, long end, long incr, long chunk_size, long *istart, long *iend);
void GOMP_parallel_loop_static_start (void (*fn) (void *), void *data, unsigned num_threads, long start, long end, long incr, long chunk_size);
void GOMP_parallel_loop_static(void (*fn)(void*), void* data, unsigned num_threads, long start, long end, long incr, long chunk_size, unsigned flags);
bool GOMP_loop_runtime_next(long* istart, long* iend);
bool GOMP_loop_runtime_start (long start, long end, long incr, long *istart, long *iend);
void GOMP_parallel_loop_runtime_start(void (*fn) (void *), void *data, unsigned num_threads, long start, long end, long incr);
void GOMP_parallel_loop_runtime(void (*fn)(void*), void* data, unsigned num_threads, long start, long end, long incr, unsigned flags);
void GOMP_loop_end(void);
void GOMP_loop_end_nowait(void);

/* parallel.c */

void GOMP_parallel_start(void (*fn)(void*), void* data, unsigned num_threads);
void GOMP_parallel(void (*fn)(void*), void* data, unsigned num_threads, unsigned int flags);
void GOMP_parallel_end(void);

/* task.c */

void GOMP_task(void (*fn)(void*), void* data, void (*cpyfn)(void*, void*), long arg_size, long arg_align, bool if_clause, unsigned flags, void** depend);
void GOMP_taskwait(void);

/* single.c */

bool GOMP_single_start(void);

/* omp.h */

typedef struct
{
  unsigned char _x[42]; // 42 is a bogus number. The data structure is not supported.
} omp_lock_t;

typedef struct
{
  unsigned char _x[42]; // 42 is a bogus number. The data structure is not supported.
} omp_nest_lock_t;

typedef enum omp_sched_t
{
  omp_sched_static = 1,
  omp_sched_dynamic = 2,
  omp_sched_guided = 3,
  omp_sched_auto = 4
} omp_sched_t;

typedef enum omp_proc_bind_t
{
  omp_proc_bind_false = 0,
  omp_proc_bind_true = 1,
  omp_proc_bind_master = 2,
  omp_proc_bind_close = 3,
  omp_proc_bind_spread = 4
} omp_proc_bind_t;

void omp_set_num_threads (int);
int omp_get_num_threads (void);
int omp_get_max_threads (void);
int omp_get_thread_num (void);
int omp_get_num_procs (void);

int omp_in_parallel (void);

void omp_set_dynamic (int);
int omp_get_dynamic (void);

void omp_set_nested (int);
int omp_get_nested (void);

void omp_init_lock (omp_lock_t *);
void omp_destroy_lock (omp_lock_t *);
void omp_set_lock (omp_lock_t *);
void omp_unset_lock (omp_lock_t *);
int omp_test_lock (omp_lock_t *);

void omp_init_nest_lock (omp_nest_lock_t *);
void omp_destroy_nest_lock (omp_nest_lock_t *);
void omp_set_nest_lock (omp_nest_lock_t *);
void omp_unset_nest_lock (omp_nest_lock_t *);
int omp_test_nest_lock (omp_nest_lock_t *);

double omp_get_wtime (void);
double omp_get_wtick (void);

void omp_set_schedule (omp_sched_t, int);
void omp_get_schedule (omp_sched_t *, int *);
int omp_get_thread_limit (void);
void omp_set_max_active_levels (int);
int omp_get_max_active_levels (void);
int omp_get_level (void);
int omp_get_ancestor_thread_num (int);
int omp_get_team_size (int);
int omp_get_active_level (void);

int omp_in_final (void);

int omp_get_cancellation (void);
omp_proc_bind_t omp_get_proc_bind (void);

void omp_set_default_device (int);
int omp_get_default_device (void);
int omp_get_num_devices (void);
int omp_get_num_teams (void);
int omp_get_team_num (void);

int omp_is_initial_device (void);

END_C_DECLS
#endif
