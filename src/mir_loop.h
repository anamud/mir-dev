#ifndef MIR_LOOP_H
#define MIR_LOOP_H 1

#include <stdint.h>

#include "mir_lock.h"

/*LIBINT_DECL_BEGIN*/
struct mir_loop_des_t 
{
    long end;
    long incr;
    long chunk_size;
    long next;
    long static_trip;
    struct mir_lock_t lock;
};
/*LIBINT_DECL_END*/

#endif