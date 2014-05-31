#ifndef MIR_OMP_INT_H
#define MIR_OMP_INT_H

#include <stdbool.h>
#include "mir_types.h"

BEGIN_C_DECLS 

/* Taken from https://gcc.gnu.org/git/?p=gcc.git;a=tree;f=libgomp;hb=HEAD on 21 May 2014*/

/* barrier.c */

void GOMP_barrier (void);
bool GOMP_barrier_cancel (void);

/* critical.c */

void GOMP_critical_start (void);
void GOMP_critical_end (void);
void GOMP_critical_name_start (void **);
void GOMP_critical_name_end (void **);
void GOMP_atomic_start (void);
void GOMP_atomic_end (void);

/* loop.c */

bool GOMP_loop_static_start (long, long, long, long, long *, long *);
bool GOMP_loop_dynamic_start (long, long, long, long, long *, long *);
bool GOMP_loop_guided_start (long, long, long, long, long *, long *);
bool GOMP_loop_runtime_start (long, long, long, long *, long *);

bool GOMP_loop_ordered_static_start (long, long, long, long, long *, long *);
bool GOMP_loop_ordered_dynamic_start (long, long, long, long, long *, long *);
bool GOMP_loop_ordered_guided_start (long, long, long, long, long *, long *);
bool GOMP_loop_ordered_runtime_start (long, long, long, long *, long *);

bool GOMP_loop_static_next (long *, long *);
bool GOMP_loop_dynamic_next (long *, long *);
bool GOMP_loop_guided_next (long *, long *);
bool GOMP_loop_runtime_next (long *, long *);

bool GOMP_loop_ordered_static_next (long *, long *);
bool GOMP_loop_ordered_dynamic_next (long *, long *);
bool GOMP_loop_ordered_guided_next (long *, long *);
bool GOMP_loop_ordered_runtime_next (long *, long *);

void GOMP_parallel_loop_static_start (void (*)(void *), void *, unsigned, long, long, long, long);
void GOMP_parallel_loop_dynamic_start (void (*)(void *), void *, unsigned, long, long, long, long);
void GOMP_parallel_loop_guided_start (void (*)(void *), void *, unsigned, long, long, long, long);
void GOMP_parallel_loop_runtime_start (void (*)(void *), void *, unsigned, long, long, long);
void GOMP_parallel_loop_static (void (*)(void *), void *, unsigned, long, long, long, long, unsigned);
void GOMP_parallel_loop_dynamic (void (*)(void *), void *, unsigned, long, long, long, long, unsigned);
void GOMP_parallel_loop_guided (void (*)(void *), void *, unsigned, long, long, long, long, unsigned);
void GOMP_parallel_loop_runtime (void (*)(void *), void *, unsigned, long, long, long, unsigned);

void GOMP_loop_end (void);
void GOMP_loop_end_nowait (void);
bool GOMP_loop_end_cancel (void);

/* loop_ull.c */

bool GOMP_loop_ull_static_start (bool, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long *, unsigned long long *);
bool GOMP_loop_ull_dynamic_start (bool, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long *, unsigned long long *);
bool GOMP_loop_ull_guided_start (bool, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long *, unsigned long long *);
bool GOMP_loop_ull_runtime_start (bool, unsigned long long, unsigned long long, unsigned long long, unsigned long long *, unsigned long long *); 
bool GOMP_loop_ull_ordered_static_start (bool, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long *, unsigned long long *);
bool GOMP_loop_ull_ordered_dynamic_start (bool, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long *, unsigned long long *);
bool GOMP_loop_ull_ordered_guided_start (bool, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long *, unsigned long long *);
bool GOMP_loop_ull_ordered_runtime_start (bool, unsigned long long, unsigned long long, unsigned long long, unsigned long long *, unsigned long long *);

bool GOMP_loop_ull_static_next (unsigned long long *, unsigned long long *);
bool GOMP_loop_ull_dynamic_next (unsigned long long *, unsigned long long *);
bool GOMP_loop_ull_guided_next (unsigned long long *, unsigned long long *);
bool GOMP_loop_ull_runtime_next (unsigned long long *, unsigned long long *);

bool GOMP_loop_ull_ordered_static_next (unsigned long long *, unsigned long long *);
bool GOMP_loop_ull_ordered_dynamic_next (unsigned long long *, unsigned long long *);
bool GOMP_loop_ull_ordered_guided_next (unsigned long long *, unsigned long long *);
bool GOMP_loop_ull_ordered_runtime_next (unsigned long long *, unsigned long long *);

/* ordered.c */

void GOMP_ordered_start (void);
void GOMP_ordered_end (void);

/* parallel.c */

void GOMP_parallel_start (void (*) (void *), void *, unsigned);
void GOMP_parallel_end (void);
void GOMP_parallel (void (*) (void *), void *, unsigned, unsigned);
bool GOMP_cancel (int, bool);
bool GOMP_cancellation_point (int);

/* task.c */

void GOMP_task (void (*) (void *), void *, void (*) (void *, void *), long, long, bool, unsigned, void **);
void GOMP_taskwait (void);
void GOMP_taskyield (void);
void GOMP_taskgroup_start (void);
void GOMP_taskgroup_end (void);

/* sections.c */

unsigned GOMP_sections_start (unsigned);
unsigned GOMP_sections_next (void);
void GOMP_parallel_sections_start (void (*) (void *), void *, unsigned, unsigned);
void GOMP_parallel_sections (void (*) (void *), void *, unsigned, unsigned, unsigned);
void GOMP_sections_end (void);
void GOMP_sections_end_nowait (void);
bool GOMP_sections_end_cancel (void);

/* single.c */

bool GOMP_single_start (void);
void *GOMP_single_copy_start (void);
void GOMP_single_copy_end (void *);

/* target.c */

void GOMP_target (int, void (*) (void *), const void *, size_t, void **, size_t *, unsigned char *);
void GOMP_target_data (int, const void *, size_t, void **, size_t *, unsigned char *);
void GOMP_target_end_data (void);
void GOMP_target_update (int, const void *, size_t, void **, size_t *, unsigned char *);
void GOMP_teams (unsigned int, unsigned int);

END_C_DECLS 
#endif
