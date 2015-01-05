#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "mir_public_int.h"
#include "health.h"
#include "helper.h"

int cutoff_value = 2;

long get_usecs(void)
{/*{{{*/
    struct timeval t;
    gettimeofday(&t, ((void *) 0));
    return t.tv_sec * 1000000 + t.tv_usec;
}/*}}}*/

struct main_task_wrapper_arg_t 
{/*{{{*/
    struct Village *v;
};/*}}}*/

void main_task_wrapper(void* arg)
{/*{{{*/
    struct main_task_wrapper_arg_t* warg = (struct main_task_wrapper_arg_t*) arg;
    sim_village_main_par(warg->v);
}/*}}}*/

int main(int argc, char *argv[])
{/*{{{*/
    if (argc != 2)
        PABRT("Usage: %s input-file\n", argv[0]);

    // Init the runtime
    mir_create();

    struct Village *top;
    read_input_data(argv[1]);
    allocate_village(&top, (void *)0, (void *)0, sim_level, 0);

    long par_time_start = get_usecs();
    struct main_task_wrapper_arg_t mt_arg;
    mt_arg.v = top;
    mir_task_create((mir_tfunc_t) main_task_wrapper, &mt_arg, sizeof(struct main_task_wrapper_arg_t), 0, NULL, "main_task_wrapper");
    mir_task_wait();
    long par_time_end = get_usecs();
    double par_time = (double)( par_time_end - par_time_start) / 1000000;

    int check = TEST_NOT_PERFORMED;
#ifdef CHECK_RESULT
    check = check_village(top);
#endif

    PMSG("%s(%s),check=%d in %s,time=%f secs\n", argv[0], argv[1], check, TEST_ENUM_STRING, par_time);
    PALWAYS("%fs\n", par_time);

    // Pull down the runtime
    mir_destroy();

    return 0;
}/*}}}*/
