#include "mir_perf.h" 

#ifdef __tile__
#include <arch/cycle.h>
uint64_t mir_get_cycles()
{/*{{{*/
    return get_cycle_count();
}/*}}}*/
#else
uint64_t mir_get_cycles()
{/*{{{*/
    unsigned a, d;
    __asm__ volatile("rdtsc" : "=a" (a), "=d" (d));
    return ((uint64_t)a) | (((uint64_t)d) << 32);
}/*}}}*/
#endif

