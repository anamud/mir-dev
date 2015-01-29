#ifndef MIR_OMP_INT_H
#define MIR_OMP_INT_H

#include <stdbool.h>

#include "mir_types.h"

BEGIN_C_DECLS 

/* Taken from https://gcc.gnu.org/git/?p=gcc.git;a=tree;f=libgomp;hb=HEAD on 21 May 2014*/
// FIXME: Warning!! Fragile!! This can cause a header version mismatch when linked with libgomp. Add a check! 

/* barrier.c */

void GOMP_barrier (void);

/* critical.c */

void GOMP_critical_start (void);
void GOMP_critical_end (void);

/* loop.c */

bool GOMP_loop_dynamic_start (long start, long end, long incr, long chunk_size, long *istart, long *iend);
bool GOMP_loop_dynamic_next (long *istart, long *iend);
void GOMP_parallel_loop_dynamic_start (void (*fn) (void *), void *data, unsigned num_threads, long start, long end, long incr, long chunk_size);
void GOMP_parallel_loop_dynamic (void (*fn) (void *), void *data, unsigned num_threads, long start, long end, long incr, long chunk_size, unsigned flags);

void GOMP_loop_end (void);
void GOMP_loop_end_nowait (void);

/* parallel.c */

void GOMP_parallel_start (void (*fn) (void *), void *data, unsigned num_threads);
void GOMP_parallel (void (*fn) (void *), void *data, unsigned num_threads, unsigned int flags);
void GOMP_parallel_end (void);

/* task.c */

void GOMP_task (void (*fn) (void *), void *data, void (*cpyfn) (void *, void *), long arg_size, long arg_align, bool if_clause, unsigned flags, void **depend);
void GOMP_taskwait (void);

/* single.c */

bool GOMP_single_start (void);

END_C_DECLS 
#endif
