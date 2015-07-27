#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <omp.h>
#include <sys/time.h>

static uint64_t par_res, seq_res;
int cutoff_value;
#define FIB_NUM_PRECOMP 50
uint64_t fib_results[FIB_NUM_PRECOMP] = { /*{{{*/
    0,
    1,
    1,
    2,
    3,
    5,
    8,
    13,
    21,
    34,
    55,
    89,
    144,
    233,
    377,
    610,
    987,
    1597,
    2584,
    4181,
    6765,
    10946,
    17711,
    28657,
    46368,
    75025,
    121393,
    196418,
    317811,
    514229,
    832040,
    1346269,
    2178309,
    3524578,
    5702887,
    9227465,
    14930352,
    24157817,
    39088169,
    63245986,
    102334155,
    165580141,
    267914296,
    433494437,
    701408733,
    1134903170,
    1836311903,
    2971215073,
    4807526976,
    7778742049
}; /*}}}*/

// Forward declarations
static uint64_t fib_seq(int n);
static uint64_t fib(int n, int d);
int main(int argc, char** argv);

/*static*/ uint64_t fib(int n, int d)
{ /*{{{*/
    uint64_t x, y;
    if (n < 2)
        return n;
    if (d < cutoff_value)
    {
#pragma omp task shared(x) firstprivate(n, d)
        x = fib(n - 1, d + 1);

#pragma omp task shared(y) firstprivate(n, d)
        y = fib(n - 2, d + 1);

#pragma omp taskwait
    }
    else
    {
        x = fib_seq(n - 1);
        y = fib_seq(n - 2);
    }
    return x + y;
} /*}}}*/

static uint64_t fib_seq(int n)
{ /*{{{*/
    if (n < 2)
        return n;
    return fib_seq(n - 1) + fib_seq(n - 2);
} /*}}}*/

long get_usecs(void)
{/*{{{*/
    struct timeval t;
    gettimeofday(&t, ((void *) 0));
    return t.tv_sec * 1000000 + t.tv_usec;
}/*}}}*/

int main(int argc, char** argv)
{ /*{{{*/
    if (argc > 3)
    {
        fprintf(stderr, "Usage: %s number cut_off\n", argv[0]);
        exit(1);
    }

    cutoff_value = 15;
    int num = 42;

    if (argc > 1)
        num = atoi(argv[1]);
    if (argc > 2)
        cutoff_value = atoi(argv[2]);

    fprintf(stderr, "Computing fib %d %d ...\n", num, cutoff_value);

    long par_time_start = get_usecs();

#pragma omp parallel
    {
#pragma omp single
        {
#pragma omp task
            par_res = fib(num, 0);
#pragma omp taskwait
        }
    }

    long par_time_end = get_usecs();
    double par_time = (double)(par_time_end - par_time_start) / 1000000;
    fprintf(stderr, "Execution time = %f s\n", par_time);

#ifdef CHECK_RESULT
    fprintf(stderr, "Checking ...\n");

    if (num > FIB_NUM_PRECOMP)
        seq_res = fib_seq(num);
    else {
        long seq_time_start = get_usecs();
        seq_res = fib_results[num];
        long seq_time_end = get_usecs();
        double seq_time = (double)(seq_time_end - seq_time_start) / 1000000;
        fprintf(stderr, "Seq. execution time = %f s\n", seq_time);
    }

    if (par_res == seq_res)
        fprintf(stderr, "%s(%d,%d), check result = %s\n", argv[0], num, cutoff_value, "SUCCESS");
    else
        fprintf(stderr, "%s(%d,%d), check result = %s\n", argv[0], num, cutoff_value, "FAILURE");
#endif

    return 0;
} /*}}}*/
