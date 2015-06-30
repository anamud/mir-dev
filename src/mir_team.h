#ifndef MIR_TEAM_H
#define MIR_TEAM_H 1

BEGIN_C_DECLS

/*LIBINT_BASE_DECL_BEGIN*/
struct mir_omp_team_t { /*{{{*/
    struct mir_omp_team_t* prev;
    struct mir_twc_t* barrier;
    int num_threads;
    int single_count;
}; /*}}}*/
   /*LIBINT_BASE_DECL_END*/

/*LIBINT*/ struct mir_omp_team_t* mir_new_omp_team(struct mir_omp_team_t*, unsigned);

END_C_DECLS
#endif
