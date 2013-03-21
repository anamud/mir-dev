#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include "mir_debug.h"

void mir_sleep_ms(uint32_t msec)
{/*{{{*/
#ifdef __tile__
#include <unistd.h>
    usleep(msec*1000);
#else // x86 Linux
    struct timespec timeout0;
    struct timespec timeout1;
    struct timespec* tmp;
    struct timespec* t0 = &timeout0;
    struct timespec* t1 = &timeout1;

    t0->tv_sec = msec / 1000;
    t0->tv_nsec = (msec % 1000) * (1000 * 1000);

    while ((nanosleep(t0, t1) == (-1)) && (errno == EINTR))
    {
        tmp = t0;
        t0 = t1;
        t1 = tmp;
    }
#endif
}/*}}}*/
