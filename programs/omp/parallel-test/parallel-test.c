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
    if (argc > 1)
        PMSG("Usage: %s\n", argv[0]);

    PMSG("Running %s ... \n", argv[0]);

    long par_time_start = get_usecs();
// OMP parallel in MIR is creates a singleton team of threads, and executes the parallel block on one thread.
#pragma omp parallel
{
    int result = fib_seq(1);
    fprintf(stderr, "thread %d computed fib(1) = %d\n", mir_get_threadid(), result);
// OMP parallel in MIR uses the singleton team of threads created in the parent context and executes the parallel block on one thread.
#pragma omp parallel
    {
        int result = fib_seq(2);
        fprintf(stderr, "thread %d computed fib(2) = %d\n", mir_get_threadid(), result);
// OMP parallel in MIR uses the singleton team of threads created in the parent context and executes the parallel block on one thread.
#pragma omp parallel
        {
            int result = fib_seq(3);
            fprintf(stderr, "thread %d computed fib(3) = %d\n", mir_get_threadid(), result);
        }
    }
// OMP parallel in MIR uses the singleton team of threads created in the parent context and executes the parallel block on one thread.
#pragma omp parallel
    {
        int result = fib_seq(2);
        fprintf(stderr, "thread %d computed fib(3) = %d\n", mir_get_threadid(), result);
    }
// When a parallel section ends, MIR executes a soft destroy method where resorces are released partially. Resocurces are fully released when the program calls exit(). 
}
// OMP parallel in MIR uses the singleton team of threads created by the first parallel and executes the parallel block on one thread.
#pragma omp parallel
{
    int result = fib_seq(1);
    fprintf(stderr, "thread %d computed fib(1) = %d\n", mir_get_threadid(), result);
}
    long par_time_end = get_usecs();
    double par_time = (double)( par_time_end - par_time_start) / 1000000;

    int check = TEST_NOT_PERFORMED;
    PMSG("%s, check=%d in %s,time=%f secs\n", argv[0], check, TEST_ENUM_STRING, par_time);
    PALWAYS("%fs\n", par_time);

    return 0;
}/*}}}*/
