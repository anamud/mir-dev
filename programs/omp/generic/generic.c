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
void process(int i, int n);
void test(int n);
int main(int argc, char **argv);

// Defines
#define REUSE_COUNT 1

long get_usecs(void)
{
    /*{{{*/
    struct timeval t;
    gettimeofday(&t, ((void *) 0));
    return t.tv_sec * 1000000 + t.tv_usec;
}/*}}}*/

void process(int i, int n)
{
    /*{{{*/
    printf("Processing %d-%d!\n", i, n);
}/*}}}*/

void test(int n)
{
    /*{{{*/
    for (int r = 0; r < REUSE_COUNT; r++)
    {
        for (int i = 0; i < n; i++)
        {
            #pragma omp task firstprivate(i) shared(n)
            process(i, n);
        }
        #pragma omp taskwait
    }

// Empty taskwait
    #pragma omp taskwait

// Some more tasks
    for (int i = 0; i < n; i++)
    {
        #pragma omp task firstprivate(i) shared(n)
        process(i, n);
    }
    #pragma omp taskwait
}/*}}}*/

int main(int argc, char **argv)
{
    /*{{{*/
    if (argc > 2)
        PMSG("Usage: %s number\n", argv[0]);

    int num = 42;
    if (argc > 1)
        num = atoi(argv[1]);
    PMSG("Running %s %d ... \n", argv[0], num);

    long par_time_start = get_usecs();
// OMP parallel in MIR is creates a singleton team of threads, and executes the parallel block on one thread.
    #pragma omp parallel
    {
// OMP single in MIR is dummy
        #pragma omp single
        {
            #pragma omp task
            {
                test(num);
            }
            #pragma omp taskwait
        }
    }
    long par_time_end = get_usecs();
    double par_time = (double)( par_time_end - par_time_start) / 1000000;

    int check = TEST_NOT_PERFORMED;
    PMSG("%s(%d),check=%d in %s,time=%f secs\n", argv[0], num, check, TEST_ENUM_STRING, par_time);
    PALWAYS("%fs\n", par_time);

    return 0;
}/*}}}*/
