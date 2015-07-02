#include "mir_memory.h"
#include "mir_team.h"
#include "mir_utils.h"

struct mir_omp_team_t* mir_new_omp_team(struct mir_omp_team_t* pteam, unsigned nthreads)
{ /*{{{*/
    struct mir_omp_team_t* team = mir_malloc_int(sizeof(struct mir_omp_team_t));
    MIR_ASSERT(team != NULL);
    team->prev = pteam;
    team->single_count = nthreads;
    team->num_threads = nthreads;
    mir_barrier_init(&team->barrier, team->num_threads);
    return team;
} /*}}}*/
