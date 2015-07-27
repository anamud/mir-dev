#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>

#include "mir_public_int.h"

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

typedef struct data_env_0_t_tag { /*{{{*/
    uint64_t* x_0;
    int n_0;
    int d_0;
} data_env_0_t; /*}}}*/

/*static*/ void ol_fib_0(void* arg)
{ /*{{{*/
    data_env_0_t* _args = (data_env_0_t*)arg;
    uint64_t* x_0 = (uint64_t*)(_args->x_0);
    (*x_0) = fib((_args->n_0) - 1, (_args->d_0) + 1);
} /*}}}*/

typedef struct data_env_1_t_tag { /*{{{*/
    uint64_t* y_0;
    int n_0;
    int d_0;
} data_env_1_t; /*}}}*/

/*static*/ void ol_fib_1(data_env_1_t* arg)
{ /*{{{*/
    data_env_1_t* _args = (data_env_1_t*)arg;
    uint64_t* y_0 = (uint64_t*)(_args->y_0);
    (*y_0) = fib((_args->n_0) - 2, (_args->d_0) + 1);
} /*}}}*/

typedef struct data_env_2_t_tag { /*{{{*/
    uint64_t* par_res_0;
    int n_0;
    int d_0;
} data_env_2_t; /*}}}*/

/*static*/ void ol_fib_2(data_env_2_t* arg)
{ /*{{{*/
    data_env_2_t* _args = (data_env_2_t*)arg;
    uint64_t* par_res_0 = (uint64_t*)(_args->par_res_0);
    (*par_res_0) = fib((_args->n_0), (_args->d_0));
} /*}}}*/

/*static*/ uint64_t fib(int n, int d)
{ /*{{{*/
    uint64_t x, y;
    if (n < 2)
        return n;
    if (d < cutoff_value) {
        // Create task1
        data_env_0_t imm_args_0;
        imm_args_0.x_0 = &(x);
        imm_args_0.n_0 = n;
        imm_args_0.d_0 = d;

        mir_task_create((mir_tfunc_t)ol_fib_0, (void*)&imm_args_0, sizeof(data_env_0_t), 0, NULL, "ol_fib_0");

        // Create task2
        data_env_1_t imm_args_1;
        imm_args_1.y_0 = &(y);
        imm_args_1.n_0 = n;
        imm_args_1.d_0 = d;

        mir_task_create((mir_tfunc_t)ol_fib_1, (void*)&imm_args_1, sizeof(data_env_1_t), 0, NULL, "ol_fib_1");

        // Task wait
        mir_task_wait();
    }
    else {
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
    // Init the runtime
    mir_create();

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

    // Create task
    data_env_2_t imm_args_2;
    imm_args_2.par_res_0 = &(par_res);
    imm_args_2.n_0 = num;
    imm_args_2.d_0 = 0;

    mir_task_create((mir_tfunc_t)ol_fib_2, (void*)&imm_args_2, sizeof(data_env_2_t), 0, NULL, "ol_fib_2");

    // Task wait
    mir_task_wait();

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

    // Release runtime system resources
    mir_destroy();

    return 0;
} /*}}}*/
