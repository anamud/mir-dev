#include "mir_barrier.h"

void mir_barrier_init(pthread_barrier_t * barrier, int count)
{ /*{{{*/
    pthread_barrier_init(barrier, NULL, count);
} /*}}}*/

void mir_barrier_wait(pthread_barrier_t * barrier)
{ /*{{{*/
    pthread_barrier_wait(barrier);
} /*}}}*/
