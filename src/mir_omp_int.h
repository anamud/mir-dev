#ifndef MIR_OMP_INT_H
#define MIR_OMP_INT_H

#include <stdbool.h>

#include "mir_types.h"

BEGIN_C_DECLS

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

/* loop.c */

bool GOMP_loop_dynamic_next(long* istart, long* iend);
bool GOMP_loop_dynamic_start (long start, long end, long incr, long chunk_size, long *istart, long *iend);
void GOMP_parallel_loop_dynamic(void (*fn)(void*), void* data, unsigned num_threads, long start, long end, long incr, long chunk_size, unsigned flags);
bool GOMP_loop_static_next(long* istart, long* iend);
bool GOMP_loop_static_start (long start, long end, long incr, long chunk_size, long *istart, long *iend);
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

END_C_DECLS
#endif
