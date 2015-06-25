#ifndef MIR_RUNTIME_H
#define MIR_RUNTIME_H 1

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

#include "scheduling/mir_sched_pol.h"
#include "arch/mir_arch.h"
#include "mir_omp_int.h"

BEGIN_C_DECLS

struct mir_runtime_t
{/*{{{*/
    // Data
    uint16_t num_workers;
    uint16_t* worker_cpu_map;
    pthread_key_t worker_index;
    uint64_t init_time;
    struct mir_worker_t workers[MIR_WORKER_MAX_COUNT];
    struct mir_sched_pol_t* sched_pol;
    struct mir_arch_t* arch;
    uint32_t task_inlining_limit;
    int ofp_shmid;
    char* ofp_shm;
    struct mir_twc_t* ctwc;
    unsigned int num_children_tasks;

    // Initialization control
    int init_count;
    int destroyed;

    // OpenMP support
    struct mir_lock_t omp_critsec_lock;
    enum omp_for_schedule_t omp_for_schedule;
    long omp_for_chunk_size;
    struct mir_twc_t* omp_barrier;

    // Flags
    int sig_dying;
    int enable_worker_stats;
    int enable_task_stats;
    int enable_recorder;
    int enable_ofp_handshake;
};/*}}}*/

extern struct mir_runtime_t* runtime;

/*LIBINT*/ void mir_create();

/*LIBINT*/ void mir_soft_destroy();

/*LIBINT*/ void mir_destroy();

END_C_DECLS
#endif //MIR_RUNTIME_H
