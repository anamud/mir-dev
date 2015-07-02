#ifndef MIR_BARRIER_H
#define MIR_BARRIER_H 1

#include <pthread.h>

#include "mir_types.h"

BEGIN_C_DECLS

void mir_barrier_init(pthread_barrier_t * barrier, int count);
void mir_barrier_wait(pthread_barrier_t * barrier);

END_C_DECLS

#endif
