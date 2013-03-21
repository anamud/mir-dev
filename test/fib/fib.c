#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>

#include "mir_public_int.h"

static uint64_t  par_res, seq_res;
int bots_cutoff_value;
#define FIB_NUM_PRECOMP 50
uint64_t fib_results[FIB_NUM_PRECOMP] = 
{/*{{{*/
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
};/*}}}*/

// Forward declarations
uint64_t fib_seq (int n);
uint64_t fib(int n, int d);
long bots_usecs(void);
int main(int argc, char **argv);

typedef struct _nx_data_env_0_t_tag
{/*{{{*/
        uint64_t *x_0;
        int n_0;
        int d_0;
} _nx_data_env_0_t;/*}}}*/

static void _smp__ol_fib_0(void* arg)
{/*{{{*/
    _nx_data_env_0_t * _args = (_nx_data_env_0_t*) arg;
    uint64_t *x_0 = (uint64_t  *) (_args->x_0);
    (*x_0) = fib((_args->n_0) - 1, (_args->d_0) + 1);
}/*}}}*/

typedef struct _nx_data_env_1_t_tag
{/*{{{*/
        uint64_t *y_0;
        int n_0;
        int d_0;
} _nx_data_env_1_t;/*}}}*/

static void _smp__ol_fib_1(_nx_data_env_1_t * arg)
{/*{{{*/
    _nx_data_env_1_t *_args = (_nx_data_env_1_t* ) arg;
    uint64_t  *y_0 = (uint64_t  *) (_args->y_0);
    (*y_0) = fib((_args->n_0) - 2, (_args->d_0) + 1);
}/*}}}*/

uint64_t  fib(int n, int d)
{/*{{{*/
    uint64_t  x, y;
    if (n < 2)
        return n;
    if (d < bots_cutoff_value)
    {/*{{{*/
        // Create task1
        _nx_data_env_0_t imm_args_0;
        imm_args_0.x_0 = &(x);
        imm_args_0.n_0 = n;
        imm_args_0.d_0 = d;

        struct mir_task_t* task_0 = mir_task_create((mir_tfunc_t) _smp__ol_fib_0, (void*) &imm_args_0, sizeof(_nx_data_env_0_t), NULL, 0, NULL, NULL);
        
        // Create task2
        _nx_data_env_1_t imm_args_1;
        imm_args_1.y_0 = &(y);
        imm_args_1.n_0 = n;
        imm_args_1.d_0 = d;

        struct mir_task_t* task_1 = mir_task_create((mir_tfunc_t) _smp__ol_fib_1, (void*) &imm_args_1, sizeof(_nx_data_env_1_t), NULL, 0, NULL, NULL);

        // Wait for two tasks
        mir_task_wait(task_0);
        mir_task_wait(task_1);
    }/*}}}*/
    else
    {/*{{{*/
        x = fib_seq(n - 1);
        y = fib_seq(n - 2);
    }/*}}}*/

    return x + y;
}/*}}}*/

long bots_usecs(void)
{/*{{{*/
    struct timeval t;
    gettimeofday(&t, ((void *) 0));
    return t.tv_sec * 1000000 + t.tv_usec;
}/*}}}*/

uint64_t  fib_seq (int n)
{/*{{{*/
if (n<2) return n;
return fib_seq(n-1) + fib_seq(n-2);
}/*}}}*/

int main(int argc, char **argv)
{/*{{{*/
    // Init the runtime
    mir_create();

    if (argc > 3)
    {
        printf("Usage: fib number cut_off\n");
        exit(0);
    }

    bots_cutoff_value = 15;
    int num = 42;

    if(argv[2])
        bots_cutoff_value = atoi(argv[2]);
    if(argv[1])
        num = atoi(argv[1]);
    printf("Computing fib %d %d ... \n", num, bots_cutoff_value);

    long par_start = bots_usecs();
    par_res = fib(num, 0);
    long par_end = bots_usecs();

    if (num > FIB_NUM_PRECOMP)
        seq_res = fib_seq(num);
    else
        seq_res = fib_results[num];

    if (par_res == seq_res)
        printf("fib,arg(%d,%d),%s,%f sec\n", num, bots_cutoff_value, "correct", ((double) (par_end - par_start)) / 1000000);
    else
        printf("fib,arg(%d,%d),%s,%f sec\n", num, bots_cutoff_value, "INCORRECT", ((double) (par_end - par_start)) / 1000000);

    mir_destroy();

    return 0;
}/*}}}*/
