#include "mir_utils.h"
#include "mir_runtime.h"
#include "mir_worker.h"
#include "mir_defines.h"

#include <sys/resource.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>

int mir_pstack_set_size(size_t sz)
{ /*{{{*/
    MIR_CONTEXT_ENTER;

    struct rlimit rl;

    int result = getrlimit(RLIMIT_STACK, &rl);
    if (result == 0) {
        if (rl.rlim_cur < sz) {
            rl.rlim_cur = sz;
            result = setrlimit(RLIMIT_STACK, &rl);
        }
    }

    MIR_CONTEXT_EXIT; return result;
} /*}}}*/

int mir_get_num_threads()
{ /*{{{*/
    return runtime->num_workers;
} /*}}}*/

int mir_get_threadid()
{ /*{{{*/
    MIR_CONTEXT_ENTER;

    struct mir_worker_t* worker = mir_worker_get_context();

    MIR_CONTEXT_EXIT; return worker->id;
} /*}}}*/

void mir_sleep_ms(uint32_t msec)
{ /*{{{*/
    MIR_CONTEXT_ENTER;

#ifdef __tile__
#include <unistd.h>
    usleep(msec * 1000);
#else // x86 Linux
    struct timespec timeout0;
    struct timespec timeout1;
    struct timespec* tmp;
    struct timespec* t0 = &timeout0;
    struct timespec* t1 = &timeout1;

    t0->tv_sec = msec / 1000;
    t0->tv_nsec = (msec % 1000) * (1000 * 1000);

    while ((nanosleep(t0, t1) == (-1)) && (errno == EINTR)) {
        tmp = t0;
        t0 = t1;
        t1 = tmp;
    }
#endif

    MIR_CONTEXT_EXIT;
} /*}}}*/

void mir_sleep_us(uint32_t usec)
{ /*{{{*/
    MIR_CONTEXT_ENTER;

#ifdef __tile__
#include <unistd.h>
    usleep(usec);
#else // x86 Linux
    struct timespec timeout0;
    struct timespec timeout1;
    struct timespec* tmp;
    struct timespec* t0 = &timeout0;
    struct timespec* t1 = &timeout1;
    //
    // FIXME: Check this!
    t0->tv_sec = usec / (1000 * 1000);
    t0->tv_nsec = (usec % (1000 * 1000)) * (1000);

    while ((nanosleep(t0, t1) == (-1)) && (errno == EINTR)) {
        tmp = t0;
        t0 = t1;
        t1 = tmp;
    }
#endif

    MIR_CONTEXT_EXIT;
} /*}}}*/

#ifdef __tile__
#include <arch/cycle.h>
uint64_t mir_get_cycles()
{ /*{{{*/
    MIR_CONTEXT_ENTER;

    uint64_t count = get_cycle_count();

    MIR_CONTEXT_EXIT; return count;
} /*}}}*/
#else
uint64_t mir_get_cycles()
{ /*{{{*/
    MIR_CONTEXT_ENTER;

    unsigned a, d;
    __asm__ volatile("rdtsc"
                     : "=a"(a), "=d"(d));

    MIR_CONTEXT_EXIT; return ((uint64_t)a) | (((uint64_t)d) << 32);
} /*}}}*/
#endif
