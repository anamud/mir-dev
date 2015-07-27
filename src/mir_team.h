#ifndef MIR_TEAM_H
#define MIR_TEAM_H 1

#include "mir_barrier.h"
#include "mir_lock.h"

BEGIN_C_DECLS

typedef struct mir_omp_team_t mir_omp_team_t;

struct mir_omp_team_t { /*{{{*/
    struct mir_omp_team_t* prev;
    pthread_barrier_t barrier;
    int barrier_impending_count;
    int num_threads;
    int single_count;
    struct mir_lock_t loop_lock;
    struct mir_loop_des_t* loop;
}; /*}}}*/

struct mir_omp_team_t* mir_new_omp_team(struct mir_omp_team_t*, unsigned);

END_C_DECLS
#endif
