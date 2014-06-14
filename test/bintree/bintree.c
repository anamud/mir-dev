#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>

#include "mir_public_int.h"
#include "helper.h"

/*static*/ uint64_t  par_res;
int cutoff, work;

// Forward declarations
/*static*/ uint64_t fib(int n);
/*static*/ uint64_t node(int d);
int main(int argc, char **argv);

typedef struct data_env_0_t_tag
{/*{{{*/
        uint64_t *x_0;
        int d_0;
} data_env_0_t;/*}}}*/

/*static*/ void ol_node_0(void* arg)
{/*{{{*/
    data_env_0_t * _args = (data_env_0_t*) arg;
    uint64_t *x_0 = (uint64_t  *) (_args->x_0);
    (*x_0) = node((_args->d_0) + 1);
}/*}}}*/

typedef struct data_env_1_t_tag
{/*{{{*/
        uint64_t *y_0;
        int d_0;
} data_env_1_t;/*}}}*/

/*static*/ void ol_node_1(data_env_1_t * arg)
{/*{{{*/
    data_env_1_t *_args = (data_env_1_t* ) arg;
    uint64_t  *y_0 = (uint64_t  *) (_args->y_0);
    (*y_0) = node((_args->d_0) + 1);
}/*}}}*/

/*static*/ uint64_t node(int d)
{/*{{{*/
    uint64_t  x = 0, y = 0;
    if (d < cutoff)
    {/*{{{*/
        // Create task1
        data_env_0_t imm_args_0;
        imm_args_0.x_0 = &(x);
        imm_args_0.d_0 = d;

        struct mir_task_t* task_0 = mir_task_create((mir_tfunc_t) ol_node_0, (void*) &imm_args_0, sizeof(data_env_0_t), 0, NULL, NULL);
        
        // Create task2
        data_env_1_t imm_args_1;
        imm_args_1.y_0 = &(y);
        imm_args_1.d_0 = d;

        struct mir_task_t* task_1 = mir_task_create((mir_tfunc_t) ol_node_1, (void*) &imm_args_1, sizeof(data_env_1_t), 0, NULL, NULL);

        // Task wait
        mir_twc_wait();
    }/*}}}*/
    else
    {/*{{{*/
        x = fib(work);
    }/*}}}*/

    return x + y;
}/*}}}*/

/*static*/ uint64_t fib(int n)
{/*{{{*/
    uint64_t  x, y;
    if (n < 2)
        return n;

    x = fib(n-1);
    y = fib(n-2);

    return x + y;
}/*}}}*/

long get_usecs(void)
{/*{{{*/
    struct timeval t;
    gettimeofday(&t, ((void *) 0));
    return t.tv_sec * 1000000 + t.tv_usec;
}/*}}}*/

int main(int argc, char **argv)
{/*{{{*/
    // Init the runtime
    mir_create();

    if (argc > 3)
        PMSG("Usage: %s cutoff work\n", argv[0]);

    cutoff = 4;
    work = 36;

    if(argc > 1)
        cutoff = atoi(argv[1]);
    if(argc > 2)
        work = atoi(argv[2]);
    PMSG("Computing tree %d %d ... \n", cutoff, work);

    long par_time_start = get_usecs();
    par_res = node(0);
    long par_time_end = get_usecs();
    double par_time = (double)( par_time_end - par_time_start) / 1000000;

    int check = TEST_NOT_PERFORMED;
    PMSG("%s(%d,%d),check=%d in %s,time=%f secs\n", argv[0], cutoff, work, check, TEST_ENUM_STRING, par_time);
    PALWAYS("%fs\n", par_time);

    mir_destroy();

    return 0;
}/*}}}*/
