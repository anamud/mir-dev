#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <omp.h>

#include "mir_public_int.h"
#include "helper.h"

// Forward declarations
long get_usecs(void);
int main(int argc, char **argv);

int fib_seq (int n)
{/*{{{*/
if (n<2) return n;
return fib_seq(n-1) + fib_seq(n-2);
}/*}}}*/

long get_usecs(void)
{/*{{{*/
    struct timeval t;
    gettimeofday(&t, ((void *) 0));
    return t.tv_sec * 1000000 + t.tv_usec;
}/*}}}*/

int main(int argc, char **argv)
{/*{{{*/
    if (argc > 2)
        PMSG("Usage: %s number\n", argv[0]);

    int num = 32;
    if(argc > 1)
        num = atoi(argv[1]);
    PMSG("Running %s %d ... \n", argv[0], num);

    long par_time_start = get_usecs();
// OMP parallel in MIR is creates a singleton team of threads, and executes the parallel block on one thread.
#pragma omp parallel
{
// OMP single in MIR is dummy
#pragma omp single
{
// OMP parallel for does not create a team of threads in any case. It works only inside a parallel block.
#pragma omp parallel for schedule(runtime)
    for (int i = 0; i < 1024; ++i)
    {
        int result = fib_seq(num);
        fprintf(stderr, "iteration %d thread %d result %d \n", i, mir_get_threadid(), result);
    }
}
}
    long par_time_end = get_usecs();
    double par_time = (double)( par_time_end - par_time_start) / 1000000;

    int check = TEST_NOT_PERFORMED;
    PMSG("%s(%d),check=%d in %s,time=%f secs\n", argv[0], num, check, TEST_ENUM_STRING, par_time);
    PALWAYS("%fs\n", par_time);

    return 0;
}/*}}}*/
