#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <omp.h>
#include <sys/time.h>

int fib_seq (int n)
{/*{{{*/
if (n<2)
    return n;
else
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
    if (argc > 3)
        fprintf(stderr, "Usage: %s num_iterations work_intensity\n", argv[0]);

    int num_iter = 512;
    int work_int = 32;
    if(argc > 1)
        num_iter = atoi(argv[1]);
    if(argc > 2)
        work_int = atoi(argv[2]);

    fprintf(stderr, "Running %s %d %d ...\n", argv[0], num_iter, work_int);

    long par_time_start = get_usecs();
#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < num_iter; i++)
    {
        int result = fib_seq(work_int);
        fprintf(stderr, "... iteration %d thread %d result = %d\n", i, omp_get_thread_num(), result);
    }
    long par_time_end = get_usecs();
    double par_time = (double)( par_time_end - par_time_start) / 1000000;
    fprintf(stderr, "Execution time = %f s\n", par_time);

#ifdef CHECK_RESULT
    fprintf(stderr, "%s(%d,%d), check result = %s\n", argv[0], num_iter, work_int, "CHECK_NOT_PERFORMED");
#endif

    return 0;
}/*}}}*/
