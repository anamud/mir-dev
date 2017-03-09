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
    struct rlimit rl;

    int result = getrlimit(RLIMIT_STACK, &rl);
    if (result == 0) {
        if (rl.rlim_cur < sz) {
            rl.rlim_cur = sz;
            result = setrlimit(RLIMIT_STACK, &rl);
        }
    }

    return result;
} /*}}}*/

int mir_get_num_threads()
{ /*{{{*/
    return runtime->num_workers;
} /*}}}*/

int mir_get_threadid()
{ /*{{{*/
    struct mir_worker_t* worker = mir_worker_get_context();

    return worker->id;
} /*}}}*/

void mir_sleep_ms(uint32_t msec)
{ /*{{{*/
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
} /*}}}*/

void mir_sleep_us(uint32_t usec)
{ /*{{{*/
#ifdef __tile__
#include <unistd.h>
    usleep(usec);
#else // x86 Linux
    struct timespec timeout0;
    struct timespec timeout1;
    struct timespec* tmp;
    struct timespec* t0 = &timeout0;
    struct timespec* t1 = &timeout1;

    t0->tv_sec = usec / (1000 * 1000);
    t0->tv_nsec = (usec % (1000 * 1000)) * (1000);

    while ((nanosleep(t0, t1) == (-1)) && (errno == EINTR)) {
        tmp = t0;
        t0 = t1;
        t1 = tmp;
    }
#endif
} /*}}}*/

#ifdef __tile__
#include <arch/cycle.h>
uint64_t mir_get_cycles()
{ /*{{{*/
    uint64_t count = get_cycle_count();

    return count;
} /*}}}*/
#else
uint64_t mir_get_cycles()
{ /*{{{*/
    unsigned a, d;
    // Serialize before calling RDTSC
    // See: http://www.intel.com/content/dam/www/public/us/en/documents/white-papers/ia-32-ia-64-benchmark-code-execution-paper.pdf
    __asm__ volatile(
        "xor %%eax,%%eax\n\t"
        "cpuid\n\t"
        "rdtsc\n\t"
        "movl %%eax, %0\n\t"
        "movl %%edx, %1\n\t"
        : "=r" (a), "=r" (d)
        :
        : "%eax","%ebx","%ecx","%edx");

    return ((uint64_t)a) | (((uint64_t)d) << 32);
} /*}}}*/
#endif

/* This function is called upon every function entry
 * when code is compiled using -finstrument-functions. */
void __cyg_profile_func_enter(void *func, void *callsite)
{/*{{{*/
    MIR_CONTEXT_ENTER;
}/*}}}*/

/* This function is called upon every function exit
 * when code is compiled using -finstrument-functions. */
void __cyg_profile_func_exit(void *func, void *callsite)
{/*{{{*/
    MIR_CONTEXT_EXIT;
}/*}}}*/
