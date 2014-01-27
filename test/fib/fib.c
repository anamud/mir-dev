#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>

#include "mir_public_int.h"
#include "helper.h"

static uint64_t  par_res, seq_res;
int cutoff_value;
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
static uint64_t fib_seq (int n);
static uint64_t fib(int n, int d);
int main(int argc, char **argv);

typedef struct data_env_0_t_tag
{/*{{{*/
        uint64_t *x_0;
        int n_0;
        int d_0;
} data_env_0_t;/*}}}*/

/*static*/ void ol_fib_0(void* arg)
{/*{{{*/
    data_env_0_t * _args = (data_env_0_t*) arg;
    uint64_t *x_0 = (uint64_t  *) (_args->x_0);
    (*x_0) = fib((_args->n_0) - 1, (_args->d_0) + 1);
}/*}}}*/

typedef struct data_env_1_t_tag
{/*{{{*/
        uint64_t *y_0;
        int n_0;
        int d_0;
} data_env_1_t;/*}}}*/

/*static*/ void ol_fib_1(data_env_1_t * arg)
{/*{{{*/
    data_env_1_t *_args = (data_env_1_t* ) arg;
    uint64_t  *y_0 = (uint64_t  *) (_args->y_0);
    (*y_0) = fib((_args->n_0) - 2, (_args->d_0) + 1);
}/*}}}*/

typedef struct data_env_2_t_tag
{/*{{{*/
        uint64_t *par_res_0;
        int n_0;
        int d_0;
} data_env_2_t;/*}}}*/

/*static*/ void ol_fib_2(data_env_2_t * arg)
{/*{{{*/
    data_env_2_t *_args = (data_env_2_t* ) arg;
    uint64_t  *par_res_0 = (uint64_t  *) (_args->par_res_0);
    (*par_res_0) = fib((_args->n_0), (_args->d_0));
}/*}}}*/

/*static*/ uint64_t fib(int n, int d)
{/*{{{*/
    uint64_t  x, y;
    if (n < 2)
        return n;
    if (d < cutoff_value)
    {/*{{{*/
#ifndef IMPLICIT_TASK_WAIT
        struct mir_twc_t* twc = mir_twc_create();
#endif

        // Create task1
        data_env_0_t imm_args_0;
        imm_args_0.x_0 = &(x);
        imm_args_0.n_0 = n;
        imm_args_0.d_0 = d;

#ifdef IMPLICIT_TASK_WAIT
        struct mir_task_t* task_0 = mir_task_create_pw((mir_tfunc_t) ol_fib_0, (void*) &imm_args_0, sizeof(data_env_0_t), 0, NULL, NULL);
#else
        struct mir_task_t* task_0 = mir_task_create((mir_tfunc_t) ol_fib_0, (void*) &imm_args_0, sizeof(data_env_0_t), /*NULL*/twc, 0, NULL, NULL);
#endif
        
        // Create task2
        data_env_1_t imm_args_1;
        imm_args_1.y_0 = &(y);
        imm_args_1.n_0 = n;
        imm_args_1.d_0 = d;

#ifdef IMPLICIT_TASK_WAIT
        struct mir_task_t* task_1 = mir_task_create_pw((mir_tfunc_t) ol_fib_1, (void*) &imm_args_1, sizeof(data_env_1_t), 0, NULL, NULL);
#else
        struct mir_task_t* task_1 = mir_task_create((mir_tfunc_t) ol_fib_1, (void*) &imm_args_1, sizeof(data_env_1_t), /*NULL*/twc, 0, NULL, NULL);
#endif

#ifdef IMPLICIT_TASK_WAIT
        // Task wait
        mir_twc_wait_pw();
#else
        //// Wait for two tasks
        //mir_task_wait(task_0);
        //mir_task_wait(task_1);
        // Task wait
        mir_twc_wait(twc);
#endif
    }/*}}}*/
    else
    {/*{{{*/
        /*char s[256] = {};*/
        /*for(int j=0; j<d; j++)*/
            /*s[j] = '-';*/
        /*s[d] = '\0';*/
        /*PALWAYS("%s(%d)\n", s, n-1);*/
        x = fib_seq(n - 1);
        /*PALWAYS("%s(%d)\n", s, n-2);*/
        y = fib_seq(n - 2);
    }/*}}}*/

    return x + y;
}/*}}}*/

long get_usecs(void)
{/*{{{*/
    struct timeval t;
    gettimeofday(&t, ((void *) 0));
    return t.tv_sec * 1000000 + t.tv_usec;
}/*}}}*/

static uint64_t fib_seq (int n)
{/*{{{*/
if (n<2) return n;
return fib_seq(n-1) + fib_seq(n-2);
}/*}}}*/

static int check_fib()
{/*{{{*/
    if (par_res == seq_res)
        return TEST_SUCCESSFUL;
    else
        return TEST_UNSUCCESSFUL;
}/*}}}*/

int main(int argc, char **argv)
{/*{{{*/
    // Init the runtime
    mir_create();

    if (argc > 3)
        PMSG("Usage: %s number cut_off\n", argv[0]);

    cutoff_value = 15;
    int num = 42;

    if(argc > 1)
        num = atoi(argv[1]);
    if(argc > 2)
        cutoff_value = atoi(argv[2]);
    PMSG("Computing fib %d %d ... \n", num, cutoff_value);

    long par_time_start = get_usecs();
    // --- IMPLICIT TASK ---
    struct mir_twc_t* twc = mir_twc_create();

    // Create task
    data_env_2_t imm_args_2;
    imm_args_2.par_res_0 = &(par_res);
    imm_args_2.n_0 = num;
    imm_args_2.d_0 = 0;

    struct mir_task_t* task_2 = mir_task_create((mir_tfunc_t) ol_fib_2, (void*) &imm_args_2, sizeof(data_env_2_t), /*NULL*/twc, 0, NULL, NULL);
    
    // Task wait
    mir_twc_wait(twc);
    // --- NO IMPLICIT TASK ---
    //par_res = fib(num, 0);
    // ---
    long par_time_end = get_usecs();
    double par_time = (double)( par_time_end - par_time_start) / 1000000;

#ifdef SHOW_NUMA_STATS
    char cmd[256];
    int pid = getpid();
    sprintf(cmd, "~/survival_tools/bin/nmstat %d > numa_stats-%d\n", pid, pid);
    system(cmd);
    sprintf(cmd, "cp /proc/%d/maps maps-%d\n", pid, pid);
    system(cmd);
#endif

    int check = TEST_NOT_PERFORMED;
#ifdef CHECK_RESULT
    PDBG("Checking ... \n");
    if (num > FIB_NUM_PRECOMP)
        seq_res = fib_seq(num);
    else
    {
        long seq_time_start = get_usecs();
        seq_res = fib_results[num];
        long seq_time_end = get_usecs();
        double seq_time = (double)( seq_time_end - seq_time_start) / 1000000;
        PMSG("Seq. time=%f secs\n", seq_time);
    }
    check = check_fib();
#endif

    PMSG("%s(%d,%d),check=%d in %s,time=%f secs\n", argv[0], num, cutoff_value, check, TEST_ENUM_STRING, par_time);
    PALWAYS("%fs\n", par_time);

    mir_destroy();

    return 0;
}/*}}}*/
