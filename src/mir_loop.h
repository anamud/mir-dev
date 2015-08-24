#ifndef MIR_LOOP_H
#define MIR_LOOP_H 1

#include <stdbool.h>

#include "mir_lock.h"

BEGIN_C_DECLS

/*PUB_INT_DECL_BEGIN*/

struct mir_loop_schedule_t;

struct mir_loop_schedule_t {
    unsigned long chunk_start;
    unsigned long chunk_end;
    struct mir_loop_schedule_t* next;
};

struct mir_loop_des_t {
    int init;
    int non_parallel_start;
    long end;
    long incr;
    long chunk_size;
    long next;
    long static_trip;
    struct mir_lock_t lock;
    struct mir_loop_schedule_t* precomp_schedule;
    bool precomp_schedule_exists;
};
/*PUB_INT_DECL_END*/

struct mir_loop_des_t* mir_new_omp_loop_desc();
void mir_omp_loop_desc_init(struct mir_loop_des_t* loop, long start, long end, long incr, long chunk_size, bool use_precomp_schedule);
struct mir_loop_des_t* mir_new_omp_loop_desc_init(long start, long end, long incr, long chunk_size, bool use_precomp_schedule);

END_C_DECLS
#endif
