#include "mir_memory.h"
#include "mir_team.h"
#include "mir_utils.h"

struct mir_omp_team_t* mir_new_omp_team(struct mir_omp_team_t* pteam, unsigned nthreads)
{ /*{{{*/
    struct mir_omp_team_t* team = mir_malloc_int(sizeof(struct mir_omp_team_t));
    MIR_CHECK_MEM(team != NULL);
    team->prev = pteam;
    team->single_count = nthreads;
    team->num_threads = nthreads;
    mir_barrier_init(&team->barrier, team->num_threads);
    team->barrier_impending_count = 0;
    mir_lock_create(&(team->loop_lock));
    team->loop = NULL;
    for (int i = 0; i < MIR_WORKER_MAX_COUNT; i++)
        team->parallel_block_flag[i] = 0;
    return team;
} /*}}}*/
