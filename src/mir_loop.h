#ifndef MIR_LOOP_H
#define MIR_LOOP_H 1

#include "mir_lock.h"

/*LIBINT_DECL_BEGIN*/
struct mir_loop_des_t {
    int init;
    long end;
    long incr;
    long chunk_size;
    long next;
    long static_trip;
    struct mir_lock_t lock;
};
/*LIBINT_DECL_END*/

/*LIBINT*/ struct mir_loop_des_t* mir_new_omp_loop_desc();

#endif
