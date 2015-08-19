#include "mir_barrier.h"
#include "mir_defines.h"

void mir_barrier_init(pthread_barrier_t * barrier, int count)
{ /*{{{*/
    MIR_CONTEXT_ENTER;

    pthread_barrier_init(barrier, NULL, count);

    MIR_CONTEXT_EXIT;
} /*}}}*/

// The function mir_barrier_wait returns 1 to a single worker.
// How? From the pthread_barrier_wait man page:
// When the required number of threads have called pthread_barrier_wait() specifying the barrier, the constant PTHREAD_BARRIER_SERIAL_THREAD shall be returned to one unspecified thread and zero shall be returned to each of the remaining threads.

int mir_barrier_wait(pthread_barrier_t * barrier)
{ /*{{{*/
    MIR_CONTEXT_ENTER;

    if(PTHREAD_BARRIER_SERIAL_THREAD == pthread_barrier_wait(barrier)) {
        MIR_CONTEXT_EXIT_VAL (1);
    }
    else {
        MIR_CONTEXT_EXIT_VAL (0);
    }
} /*}}}*/
